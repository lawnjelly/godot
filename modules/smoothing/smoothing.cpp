#include "smoothing.h"

void Smoothing::add(int value) {

    count += value;
}

void Smoothing::reset() {

    count = 0;
}

int Smoothing::get_total() const {

    return count;
}

void Smoothing::_bind_methods() {

    ClassDB::bind_method(D_METHOD("add", "value"), &Smoothing::add);
    ClassDB::bind_method(D_METHOD("reset"), &Smoothing::reset);
    ClassDB::bind_method(D_METHOD("get_total"), &Smoothing::get_total);
}

Smoothing::Smoothing() {
    count = 0;
}
