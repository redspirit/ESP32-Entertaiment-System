#pragma once
#include "ISubsystem.h"

class AppManager {
public:
    void setSubsystem(ISubsystem* s);
    void update(float dt);
    void tick();

private:
    ISubsystem* _current = nullptr;
};