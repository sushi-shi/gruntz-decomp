#ifndef GRUNTZ_ANIMATIONREGISTRY_H
#define GRUNTZ_ANIMATIONREGISTRY_H

#include <rva.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

class CAnimationSequence;
class CRezDir;
struct CRezItm;

class AnimationRegistry : public CWapObj {
public:
    AnimationRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}

    virtual i32 IsLoaded() OVERRIDE;
    virtual i32 IsReady() OVERRIDE;
    virtual void Unload() OVERRIDE;

    CAnimationSequence* FindAnimation(const char* key);
    void RemoveAnimation(CAnimationSequence* target);
    void ClearAnimations();
    i32 RemoveWithPrefix(const char* prefix, const char* separator);
    i32 HasWithPrefix(const char* prefix);
    CString FindAnimationKey(CAnimationSequence* target);
    virtual ~AnimationRegistry() OVERRIDE;

    CAnimationSequence* LoadAnimationFromSource(const char* key, CRezItm* source);
    CAnimationSequence* LoadAnimationFromFile(const char* key, const char* path);
    CAnimationSequence* LoadNamedAnimation(CRezItm* source);
    void AddAnimation(CAnimationSequence* animation, const char* key);
    i32 LoadFromTree(CRezDir* tree, const char* prefix, const char* separator);

    CMapStringToPtr m_animations;

private:
    void RegisterAnimation(CAnimationSequence* animation, const char* key) {
        m_animations[key] = animation;
    }
};

#endif // GRUNTZ_ANIMATIONREGISTRY_H
