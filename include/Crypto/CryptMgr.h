#ifndef GRUNTZ_CRYPTO_CRYPTMGR_H
#define GRUNTZ_CRYPTO_CRYPTMGR_H

class istream;
class ostream;

class CCryptMgr {
public:
    CCryptMgr();
    CCryptMgr(char* key);
    ~CCryptMgr();

    void SetKey(const char* key);
    void Encrypt(istream& input, ostream& output);
    void Decrypt(istream& input, ostream& output);
};

#endif // GRUNTZ_CRYPTO_CRYPTMGR_H
