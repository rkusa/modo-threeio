//
//  scene_helper.cpp
//  threeio
//
//  Created by Markus Ast on 14.02.15.
//  Copyright (c) 2015 Markus Ast. All rights reserved.
//

#include "scene_helper.h"

#include <lxidef.h>
#include <lx_action.hpp>

SceneHelper::Types::Types()
{
    rotation    = scene_service.ItemType(LXsITYPE_ROTATION);
    scale       = scene_service.ItemType(LXsITYPE_SCALE);
    translation = scene_service.ItemType(LXsITYPE_TRANSLATION);
    transform   = scene_service.ItemType(LXsITYPE_TRANSFORM);
}

void SceneHelper::BuildAxisOrder(const int rotationOrder, int axis[3])
{
    switch (rotationOrder) {
        case kRotationOrderXZY:
            axis[0] = 0;
            axis[1] = 2;
            axis[2] = 1;
            break;
            
        case kRotationOrderYXZ:
            axis[0] = 1;
            axis[1] = 0;
            axis[2] = 2;
            break;
            
        case kRotationOrderYZX:
            axis[0] = 1;
            axis[1] = 2;
            axis[2] = 0;
            break;
            
        case kRotationOrderZXY:
            axis[0] = 2;
            axis[1] = 0;
            axis[2] = 1;
            break;
            
        case kRotationOrderZYX:
            axis[0] = 2;
            axis[1] = 1;
            axis[2] = 0;
            break;
            
        case kRotationOrderXYZ:
        default:
            axis[0] = 0;
            axis[1] = 1;
            axis[2] = 2;
            break;
    }
}

void SceneHelper::ChanXform(CLxUser_Item& item, const char* channel, LXtVector vec)
{
    LxResult resultX, resultY, resultZ;
    unsigned	 indexX, indexY, indexZ;
    
    resultX = item.ChannelLookup(std::string(std::string(channel) + std::string(kChannelX)).c_str(), &indexX);
    resultY = item.ChannelLookup(std::string(std::string(channel) + std::string(kChannelY)).c_str(), &indexY);
    resultZ = item.ChannelLookup(std::string(std::string(channel) + std::string(kChannelZ)).c_str (), &indexZ);
    
    vec[0] = 0.0;
    vec[1] = 0.0;
    vec[2] = 0.0;
    
    if (LXx_OK(resultX) && LXx_OK(resultY) && LXx_OK(resultZ)) {
        CLxUser_Scene scene;
        if (!item.GetContext(scene)) {
            return;
        }
        
        CLxUser_ChannelRead chan;
        scene.GetChannels(chan, LXs_ACTIONLAYER_EDIT);
        
        chan.Double(item, indexX, &vec[0]);
        chan.Double(item, indexY, &vec[1]);
        chan.Double(item, indexZ, &vec[2]);
    }
}

bool SceneHelper::CalculateTransform(CLxUser_Item& item, LXtMatrix4& matrix)
{
    CLxUser_Scene scene;
    if (!item.GetContext(scene)) {
        return false;
    }
    
    CLxUser_ItemGraph scene_graph;
    if (!scene.GetGraph(LXsGRAPH_XFRMCORE, scene_graph)) {
        return false;
    }
    
    CLxUser_ItemGraph item_graph;
    if (!item_graph.set(scene_graph)) {
        return false;
    }
    
    CLxUser_ChannelRead chan;
    if (!scene.GetChannels(chan, LXs_ACTIONLAYER_EDIT)) {
        return false;
    }
    
    lx::Matrix4Ident(matrix);
    
    unsigned count = item_graph.Reverse(item);
    for (unsigned i = 0; i < count; ++i) {
        CLxUser_Item transform;
        
        if (!item_graph.Reverse(item, i, transform)) {
            continue;
        }
        
        if (transform.IsA(types.rotation)) {
            LXtVector rotate;
            ChanXform(transform, LXsICHAN_ROTATION_ROT, rotate);
            
            int axis[3];
            
            unsigned rotation_index;
            transform.ChannelLookup(LXsICHAN_ROTATION_ORDER, &rotation_index);
            
            int rotation_order;
            chan.Integer(transform, rotation_index, &rotation_order);
            BuildAxisOrder(rotation_order, axis);
            
            // apply axis rotations
            lx::MatrixRotate(matrix, rotate[axis[0]], axis[0]);
            lx::MatrixRotate(matrix, rotate[axis[1]], axis[0]);
            lx::MatrixRotate(matrix, rotate[axis[2]], axis[0]);
        } else if (transform.IsA(types.scale)) {
            LXtVector scale;
            ChanXform(transform, LXsICHAN_SCALE_SCL, scale);
            
            LXtMatrix4 scale_matrix;
            lx::Matrix4Ident(scale_matrix);
            
            scale_matrix[0][0] = scale[0];
            scale_matrix[1][1] = scale[1];
            scale_matrix[2][2] = scale[2];
            
            lx::Matrix4Multiply(matrix, scale_matrix);
        } else if (transform.IsA(types.translation)) {
            LXtVector translate;
            ChanXform(transform, LXsICHAN_TRANSLATION_POS, translate);
            
            LXtMatrix4 translate_matrix;
            lx::Matrix4Ident(translate_matrix);
            
            translate_matrix[3][0] = translate[0];
            translate_matrix[3][1] = translate[1];
            translate_matrix[3][2] = translate[2];
            
            lx::Matrix4Multiply(matrix, translate_matrix);
        } else if (transform.IsA(types.transform)) {
            if (!CalculateLocalTransform(transform, matrix)) {
                return false;
            }
        }
    }
    
    return true;
}

bool SceneHelper::CalculateLocalTransform(CLxUser_Item& item, LXtMatrix4& matrix) {
    LXtVector translate = {0, 0, 0};
    
    // Walk the local graph and find the translate item for
    // which this transform is the inverse.
    
    CLxUser_Scene scene;
    if (!item.GetContext(scene)) {
        return false;
    }
    
    CLxUser_ItemGraph scene_graph;
    if (!scene.GetGraph(LXsGRAPH_XFRMLOCAL, scene_graph)) {
        return false;
    }
    
    CLxUser_ItemGraph local_graph;
    if (!local_graph.set(scene_graph)) {
        return false;
    }
    
    CLxUser_Item transform;
    
    if (!local_graph.Reverse(item, 0, transform)) {
        return false;
    }
    
    if (item.IsA(types.translation)) {
        // fetch the translation vector
        ChanXform(item, LXsICHAN_TRANSLATION_POS, translate);
        
        // and bake it in...
        LXtMatrix4 translate_matrix;
        lx::Matrix4Ident(translate_matrix);
        
        // negate it translation
        translate_matrix[3][0] = -translate[0];
        translate_matrix[3][1] = -translate[1];
        translate_matrix[3][2] = -translate[2];
        
        lx::Matrix4Multiply(matrix, translate_matrix);
    }
    
    return true;
}