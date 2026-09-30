//
// Created by Ayanami on 2026/9/29.
//

#pragma once
#ifndef iGameRenameArrays_h
#define iGameRenameArrays_h

#include "iGameFilter.h"
#include "iGameDataObjectCopy.h"


IGAME_NAMESPACE_BEGIN

class RenameArrays : public Filter {
    I_OBJECT(RenameArrays);
public:
    std::string getarrayname(IGenum type,igIndex  index);
    inline bool setattrindex(IGenum type, igIndex index) {
        attrtype = type;
        attrindex = index;
        if (type == -1 || index == -1) {return false;}
        return true;
    }
    bool execute();

    protected:
    RenameArrays() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~RenameArrays() override = default;

    IGenum attrtype;
    igIndex attrindex;
};

IGAME_NAMESPACE_END
#endif //IGAMEVIS_IGAMERENAMEARRAYS_H
