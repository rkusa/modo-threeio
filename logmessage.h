//
//  threelogmessage.h
//  threeio
//
//  Created by Markus Ast on 14.02.15.
//  Copyright (c) 2015 Markus Ast. All rights reserved.
//

#ifndef __threeio__threelogmessage__
#define __threeio__threelogmessage__

#include <lxu_log.hpp>

class ThreeLogMessage : public CLxLuxologyLogMessage
{
public:
    virtual const char* GetFormat ()
    {
        return "THREE";
    }
};

#endif /* defined(__threeio__threelogmessage__) */
