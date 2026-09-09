# ResampleToLine filter 使用说明

## 1.Filter概述

ResampleToLine是一种重采样过滤器，用于将网格的点重采样至一条直线上。

输入：无结构网格

输出：包含点属性线段

## 2.调用方式

在算法处理菜单下，使用ResampleToLine选项，在弹出窗口中设置线段两个端点的坐标、重采样的数量。

### 按钮位置

![按钮位置](images/ResampleToLineImg1.png) 

### 选项卡内容

![选项卡内容](images/ResampleToLineImg2.png)



### 数据描述

**point1** 此属性控制第一个端点的坐标

**point2** 此属性控制第二个端点的坐标

**采样数量** 此属性控制线条的采样点数量

**执行**点击后生成一条含数据的线段

## 3.使用示例

测试示例位置

```powershell
/Examples/Filter/TestResampleToline.cpp
```

## 4.注意事项

只插值单元中每个点的属性，不会采样单元属性。