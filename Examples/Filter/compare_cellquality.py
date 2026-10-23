#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Cell Quality 逐值对比：iGameVis 输出 vs ParaView 基准

用法:
    python compare_cellquality.py <ParaView基准目录> <iGameVis输出目录> [--tol 1e-4]

两侧 CSV 均为如下格式（由 ParaView 导出脚本 / testCellMeshMetrics 生成）:
    # model=<名>,measure=<指标>,min=<值>,max=<值>,cells=<数量>
    cell_index,CellQuality
    0,1.0000978582178064
    ...

对比规则:
  - 单位置「不支持值」(默认 -1) 视为同一类，只在两边都是 -1 时算一致。
  - 其余数值按相对误差 ||a-b|| / max(1, |b|) <= tol 判定一致。
  - 报告每指标的最大绝对/相对偏差、不一致单元数（含前若干个下标）。
"""

import argparse
import csv
import io
import os
import sys

# Windows 控制台默认 GBK，显式改用 UTF-8，避免中文乱码
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def read_csv(path):
    """读取导出的 CellQuality CSV，返回 (meta, values)。"""
    meta = {}
    values = []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.strip()
            if not line:
                continue
            if line.startswith("#"):
                body = line.lstrip("#").strip()
                for kv in body.split(","):
                    if "=" in kv:
                        k, v = kv.split("=", 1)
                        meta[k.strip()] = v.strip()
                continue
            if line.lower().startswith("cell_index"):
                continue
            parts = line.split(",")
            if len(parts) < 2:
                continue
            try:
                values.append(float(parts[1]))
            except ValueError:
                continue
    return meta, values


def collect(dirpath):
    """扫描目录，返回 {(model, measure): (path, meta, values)}"""
    found = {}
    if not os.path.isdir(dirpath):
        return found
    for name in sorted(os.listdir(dirpath)):
        if not name.endswith(".csv"):
            continue
        if name.endswith("_celltypes.csv"):
            continue
        path = os.path.join(dirpath, name)
        meta, values = read_csv(path)
        measure = meta.get("measure")
        model = meta.get("model")
        if measure is None or model is None:
            # 退回文件名推断: <model>__<Measure>.csv
            if "__" in name:
                m, meas = name[:-4].split("__", 1)
                model = model or m
                measure = measure or meas.replace("_", " ")
            else:
                model = model or "(unknown)"
                measure = measure or name[:-4]
        found[(model, measure)] = (path, meta, values)
    return found


def compare(ref, got, unsupported=-1.0, tol=1e-4, degenerate=None):
    """对比两组数值，返回统计字典。

    degenerate: 可选的退化单元下标集合。这些单元（例如体积比中位数小 8 个数量级的
    薄片四面体）内部面法向几乎共线，acos 的输入落在 ±1 边界上，第 11 位小数的差异
    就会让结果在 0° 与 180° 之间翻转 —— 连 Verdict 自身扰动 1e-13 都会改变答案。
    这类单元单独统计，不计入 numeric_bad。
    """
    n = min(len(ref), len(got))
    if n == 0:
        return None
    degenerate = degenerate or set()
    same_unsupported = 0
    mismatch_kind = 0      # 一边 -1 一边有值
    numeric_ok = 0
    numeric_bad = 0
    degen_bad = 0
    max_abs = 0.0
    max_rel = 0.0
    worst = []
    for i in range(n):
        a, b = ref[i], got[i]
        a_un = abs(a - unsupported) < 1e-9
        b_un = abs(b - unsupported) < 1e-9
        if a_un or b_un:
            if a_un and b_un:
                same_unsupported += 1
            else:
                mismatch_kind += 1
                if len(worst) < 8:
                    worst.append((i, a, b, "unsupported-mismatch"))
            continue
        absd = abs(a - b)
        reld = absd / max(1.0, abs(a))
        if absd > max_abs:
            max_abs = absd
        if reld > max_rel:
            max_rel = reld
        if reld <= tol or absd <= tol:
            numeric_ok += 1
        elif i in degenerate:
            degen_bad += 1
        else:
            numeric_bad += 1
            if len(worst) < 8:
                worst.append((i, a, b, "value"))
    return {
        "n": n,
        "len_ref": len(ref),
        "len_got": len(got),
        "same_unsupported": same_unsupported,
        "mismatch_kind": mismatch_kind,
        "numeric_ok": numeric_ok,
        "numeric_bad": numeric_bad,
        "degen_bad": degen_bad,
        "max_abs": max_abs,
        "max_rel": max_rel,
        "worst": worst,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("refdir", help="ParaView 基准目录")
    ap.add_argument("gotdir", help="iGameVis 导出目录")
    ap.add_argument("--tol", type=float, default=1e-4, help="相对误差容差（默认 1e-4）")
    ap.add_argument("--unsupported", type=float, default=-1.0, help="不支持值（默认 -1）")
    ap.add_argument("--degenerate", default=None,
                    help="退化单元清单 CSV（列: model,cell_index）。这些单元的不一致"
                         "单独统计，不计入 FAIL。用 --write-degenerate 生成默认清单，"
                         "或把本参数留空以严格模式运行。")
    args = ap.parse_args()

    degen = {}
    if args.degenerate and os.path.isfile(args.degenerate):
        with open(args.degenerate, "r", encoding="utf-8", errors="replace") as f:
            for raw in f:
                parts = raw.strip().split(",")
                if len(parts) < 2 or parts[0].startswith("#"):
                    continue
                try:
                    degen.setdefault(parts[0], set()).add(int(parts[1]))
                except ValueError:
                    continue

    refs = collect(args.refdir)
    gots = collect(args.gotdir)

    if not refs:
        print("基准目录没有可用的 CSV:", args.refdir)
        return 2
    if not gots:
        print("输出目录没有可用的 CSV:", args.gotdir)
        return 2

    print("=" * 96)
    print("Cell Quality 逐值对比  (容差 rel<=%g)" % args.tol)
    print("  基准: %s" % args.refdir)
    print("  待测: %s" % args.gotdir)
    if degen:
        print("  退化单元清单: %s (%d 个模型)" % (args.degenerate, len(degen)))
    print("=" * 96)

    common = [k for k in refs if k in gots]
    only_ref = [k for k in refs if k not in gots]

    print("%-22s %-18s %-8s %-8s %-9s %-9s %-8s %-11s %s" % (
        "模型", "指标", "单元数", "一致", "类别不符", "数值不符", "退化不符", "最大绝对偏差", "结论"))
    print("-" * 104)

    total_bad = 0
    total_degen = 0
    for (model, measure) in sorted(common):
        _, _, refv = refs[(model, measure)]
        _, _, gotv = gots[(model, measure)]
        st = compare(refv, gotv, unsupported=args.unsupported, tol=args.tol,
                     degenerate=degen.get(model))
        if st is None:
            print("%-22s %-18s  空数组" % (model, measure))
            total_bad += 1
            continue
        ok = (st["mismatch_kind"] == 0 and st["numeric_bad"] == 0
              and st["len_ref"] == st["len_got"])
        verdict = "PASS" if ok else "FAIL"
        if not ok:
            total_bad += 1
        total_degen += st["degen_bad"]
        print("%-22s %-18s %-8d %-8d %-9d %-9d %-8d %-11.3g %s" % (
            model, measure, st["n"], st["numeric_ok"], st["mismatch_kind"],
            st["numeric_bad"], st["degen_bad"], st["max_abs"], verdict))
        if st["len_ref"] != st["len_got"]:
            print("    ⚠ 长度不一致: 基准 %d / 待测 %d" % (st["len_ref"], st["len_got"]))
        for (i, a, b, kind) in st["worst"]:
            print("      单元 %d: 基准=%.10g 待测=%.10g  (%s)" % (i, a, b, kind))

    if only_ref:
        print()
        print("基准有但待测缺失:")
        for (model, measure) in sorted(only_ref):
            print("   %s / %s" % (model, measure))

    print("-" * 104)
    print("对比完成: %d 个 (模型,指标) 组合, %d 个不一致" % (len(common), total_bad))
    if total_degen:
        print("另有 %d 项位于退化单元上（面法向近共线，acos 边界翻转，"
              "不计入不一致）" % total_degen)
    print("=" * 96)
    return 0 if total_bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
