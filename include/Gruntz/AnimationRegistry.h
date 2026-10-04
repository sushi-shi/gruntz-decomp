#ifndef GRUNTZ_ANIMATIONREGISTRY_H
#define GRUNTZ_ANIMATIONREGISTRY_H

#include <map>
#include <string>


#include <Ints.h>

#include <Ints.h>
#include <Wap32/WapObj.h>

class CAniElement;
class CRezDir;
struct CRezItm;

class AnimationRegistry : public CWapObj {
public:
    AnimationRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {}

    virtual i32 IsLoaded()  ;
    virtual i32 IsReady()  ;
    virtual void Unload()  ;

    CAniElement* FindAnimation(const std::string& key);
    void RemoveAnimation(CAniElement* target);
    void ClearAnimations();
    i32 RemoveWithPrefix(const std::string& prefix, const std::string& separator);
    i32 HasWithPrefix(const std::string& prefix);
    std::string FindAnimationKey(CAniElement* target);
    virtual ~AnimationRegistry()  ;

    CAniElement* LoadAnimationFromSource(const std::string& key, CRezItm* source);
    CAniElement* LoadAnimationFromFile(const std::string& key, const char* path);
    CAniElement* LoadNamedAnimation(CRezItm* source);
    i32 LoadFromTree(CRezDir* tree, const std::string& prefix, const std::string& separator);

    const std::map<std::string, CAniElement*>& Entries() const { return m_animations; }

private:
    std::map<std::string, CAniElement*> m_animations;
    void RegisterAnimation(CAniElement* animation, const std::string& key) {
        m_animations.insert(std::map<std::string, CAniElement*>::value_type(key, animation));
    }
};

#endif
