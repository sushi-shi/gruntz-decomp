#ifndef GRUNTZ_BUTE_ARANGE_H
#define GRUNTZ_BUTE_ARANGE_H

class CARange {
public:
    CARange() {}
    CARange(double min, double max) {
        m_min = min;
        m_max = max;
    }

    void Set(double min, double max) {
        m_min = min;
        m_max = max;
    }
    CARange Get() {
        return CARange(m_min, m_max);
    }

    double GetMin() {
        return m_min;
    }
    double GetMax() {
        return m_max;
    }

protected:
    double m_min;
    double m_max;
};

#endif // GRUNTZ_BUTE_ARANGE_H
