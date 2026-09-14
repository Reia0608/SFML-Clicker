#pragma once
#include <string>

class ISaveable
{
public:
    virtual ~ISaveable() = default;
    virtual void saveToFile(const std::string& path) = 0;
    virtual void loadFromFile(const std::string& path) = 0;
};