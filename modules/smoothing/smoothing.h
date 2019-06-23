/* smoothing.h */
#ifndef SMOOTHING_H
#define SMOOTHING_H

#include "core/reference.h"

class Smoothing : public Reference {
    GDCLASS(Smoothing, Reference);

    int count;

protected:
    static void _bind_methods();

public:
    void add(int value);
    void reset();
    int get_total() const;

    Smoothing();
};

#endif
