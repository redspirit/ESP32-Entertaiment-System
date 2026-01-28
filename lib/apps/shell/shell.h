#pragma once
#include "ISubsystem.h"
#include "keyboard.h"
#include "console.h"
#include "VGA.h"

class Shell : public ISubsystem {
    public:
        Shell(VGA& _vga);
        ~Shell();

        bool init() override;
        void update(float dt) override;
        void tick() override;

    private:
        VGA& _vga;
        Console _console;
        Keyboard _kb;
};