#ifndef GRUNTZ_BUTE_AVECTOR_H
#define GRUNTZ_BUTE_AVECTOR_H

class CAVector {
public:
    CAVector() {}
    CAVector(double i, double j, double k) {
        m_i = i;
        m_j = j;
        m_k = k;
    }

    void Set(double i, double j, double k) {
        m_i = i;
        m_j = j;
        m_k = k;
    }
    CAVector Get() {
        return CAVector(m_i, m_j, m_k);
    }

    double Geti() {
        return m_i;
    }
    double Getj() {
        return m_j;
    }
    double Getk() {
        return m_k;
    }

protected:
    double m_i;
    double m_j;
    double m_k;
};

#endif // GRUNTZ_BUTE_AVECTOR_H
