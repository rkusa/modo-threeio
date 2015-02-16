//
//  types.h
//  threeio
//
//  Created by Markus Ast on 16.02.15.
//  Copyright (c) 2015 Markus Ast. All rights reserved.
//

#ifndef __threeio__types__
#define __threeio__types__

#include <vector>
#include <map>

struct Vector3
{
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {
    }
    
    Vector3(double v[3]) : x(v[0]), y(v[1]), z(v[2]) {
    }
    
    bool operator==(const Vector3& rhs)
    {
        return (x == rhs.x &&
                y == rhs.y &&
                z == rhs.z);
    }
    
    bool operator!=(const Vector3& rhs)
    {
        return !(*this == rhs);
    }
    
    bool operator<(const Vector3& rhs) const
    {
        if (x > rhs.x) { return false; }
        if (x < rhs.x) { return true; }
        
        if (y > rhs.y) { return false; }
        if (y < rhs.y) { return true; }
        
        if (z > rhs.z) { return false; }
        if (z < rhs.z) { return true; }
        
        return false;
    }
    
    const double x, y, z;
};

struct Vertex {
    Vertex(double p[3], double n[3]) : position_(p), normal_(n) {
    }
    
    bool operator==(const Vertex& rhs)
    {
        return position_ == rhs.position() && normal_ == rhs.normal();
    }
    
    bool operator!=(const Vertex& rhs)
    {
        return !(*this == rhs);
    }
    
    bool operator<(const Vertex& rhs) const
    {
        if (position_ < rhs.position()) {
            return true;
        }
        
        if (normal_ < rhs.normal()) {
            return true;
        }
        
        return false;
    }
    
    const Vector3 position() const {
        return position_;
    }
    
    const Vector3 normal() const {
        return normal_;
    }
    
private:
    Vector3 position_;
    Vector3 normal_;
};

template <class T>
struct UniqueOrderedSet {
public:
    unsigned insert(T vector) {
        auto iter = map_.find(vector);
        if (iter != map_.end()) {
            return iter->second;
        } else {
            map_[vector] = index_;
            order_.push_back(vector);
            return index_++;
        }
    }
    
    void clear() {
        map_.clear();
        order_.clear();
        index_ = 0;
    }
    
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;
    
    iterator begin() { return order_.begin(); }
    const_iterator begin() const { return order_.begin(); }
    const_iterator cbegin() const { return order_.cbegin(); }
    iterator end() { return order_.end(); }
    const_iterator end() const { return order_.end(); }
    const_iterator cend() const { return order_.cend(); }
    
private:
    
    std::map<T, unsigned> map_;
    std::vector<T> order_; // TODO: disposed?
    unsigned index_ = 0;
};

#endif /* defined(__threeio__types__) */
