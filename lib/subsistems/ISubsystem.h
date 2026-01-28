#pragma once

class ISubsystem {
public:
    virtual ~ISubsystem() = default;

    virtual bool init() = 0;
    virtual void update(float dt) = 0;
    virtual void tick() = 0;
};
