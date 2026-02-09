#pragma once

template <typename T1, typename T2> class duple {
    T1 t1;
    T2 t2;
public:
    inline duple(const T1& t1, const T2& t2): t1(t1), t2(t2) {}
    inline ~duple(void) {}
};

template <typename T1, typename T2, typename T3> class triple {
    T1 t1;
    T2 t2;
    T3 t3;
public:
    inline triple(const T1& t1, const T2& t2, const T3& t3): t1(t1), t2(t2), t3(t3) {}
    inline ~triple(void) {}
};
