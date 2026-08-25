#ifndef GRUNTZ_ANIMATIONREGISTRY_H
#define GRUNTZ_ANIMATIONREGISTRY_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

class CAniElement;
class CSymTab;
struct CParseSource;

class AnimationRegistry : public CWapObj {
public:
    AnimationRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}

    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;

    CAniElement* FindAnimation(const char* key);
    void RemoveAnimation(CAniElement* target);
    void ClearAnimations();
    i32 RemoveWithPrefix(const char* prefix, const char* separator);
    i32 HasWithPrefix(const char* prefix);
    CString FindAnimationKey(CAniElement* target);
    virtual ~AnimationRegistry() OVERRIDE;

    CAniElement* LoadAnimationFromSource(const char* key, CParseSource* source);
    CAniElement* LoadAnimationFromFile(const char* key, const char* path);
    CAniElement* LoadNamedAnimation(CParseSource* source);
    void AddAnimation(CAniElement* animation, const char* key);
    i32 LoadFromTree(CSymTab* tree, const char* prefix, const char* separator);

    CMapStringToPtr m_animations;
};

#endif // GRUNTZ_ANIMATIONREGISTRY_H
