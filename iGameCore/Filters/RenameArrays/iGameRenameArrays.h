//
// Created by Ayanami on 2026/9/29.
//

#pragma once
#ifndef iGameRenameArrays_h
#define iGameRenameArrays_h

#include "iGameFilter.h"


IGAME_NAMESPACE_BEGIN

class RenameArrays : public Filter {
    I_OBJECT(RenameArrays);
public:
    std::string getarrayname(IGenum type,igIndex  index) {

    }

    protected:
    RenameArrays() {
        SetNumberOfInputs(1);
        SetNumberOfOutputs(1);
    }
    ~RenameArrays() override = default;
};

IGAME_NAMESPACE_END
#endif //IGAMEVIS_IGAMERENAMEARRAYS_H
