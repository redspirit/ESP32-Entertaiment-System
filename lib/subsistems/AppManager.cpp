#include "AppManager.h"

void AppManager::setSubsystem(ISubsystem* s) {
    if (_current) {
        delete _current;
    }
    _current = s;
    if (_current) {
        _current->init();
    }
}

void AppManager::update(float dt) {
    if (_current) {
        _current->update(dt);
    }
}

void AppManager::tick() {
    if (_current) {
        _current->tick();
    }
}
