#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>

#include <stdio.h>
#include <string.h>

i32 AnimationRegistry::IsReady() {
    return 1;
}

void AnimationRegistry::Unload() {
    ClearAnimations();
}

void AnimationRegistry::RemoveAnimation(CAniElement* target) {
    if (target == NULL) {
        return;
    }
    std::map<std::string, CAniElement*>::iterator pos = m_animations.begin();
    std::string key;
    CAniElement* animation = NULL;
    while (pos != m_animations.end()) {
        (key = pos->first, animation = pos->second, ++pos);
        if (target == animation) {
            m_animations.erase(key);
            delete target;
            return;
        }
    }
}

void AnimationRegistry::ClearAnimations() {
    std::map<std::string, CAniElement*>::iterator pos = m_animations.begin();
    std::string key;
    CAniElement* animation = NULL;
    if (pos != m_animations.end()) {
        do {
            (key = pos->first, animation = pos->second, ++pos);
            if (animation != NULL) {
                delete animation;
            }
        } while (pos != m_animations.end());
    }
    m_animations.clear();
}

i32 AnimationRegistry::RemoveWithPrefix(const std::string& prefix, const std::string& separator) {
    std::string match(prefix);
    match += separator;
    i32 prefixLength = static_cast<i32>((match).size());
    std::string key;
    CAniElement* animation = NULL;
    std::map<std::string, CAniElement*>::iterator pos = m_animations.begin();
    i32 removedCount = 0;
    while (pos != m_animations.end()) {
        (key = pos->first, animation = pos->second, ++pos);
        if (key.compare(0, prefixLength, match) == 0) {
            m_animations.erase(key);
            if (animation != NULL) {
                delete animation;
            }
            ++removedCount;
        }
    }
    return removedCount;
}

CAniElement* AnimationRegistry::LoadAnimationFromSource(const std::string& key, CRezItm* source) {
    CAniElement* existing = FindAnimation(key);
    if (existing != NULL) return existing;
    CAniElement* animation = new CAniElement;
    if (animation == NULL) {
        return NULL;
    }
    if (animation->Configure(OwnerMgr()->SoundRegistry(), source, 0) == 0) {

        delete animation;
        return NULL;
    }
    RegisterAnimation(animation, key);
    return animation;
}

CAniElement* AnimationRegistry::LoadAnimationFromFile(const std::string& key, const char* path) {
    CAniElement* existing = FindAnimation(key);
    if (existing != NULL) return existing;
    CAniElement* animation = new CAniElement;
    if (animation == NULL) {
        return NULL;
    }
    if (animation->LoadFile(OwnerMgr()->SoundRegistry(), path, 0) == 0) {

        delete animation;
        return NULL;
    }
    RegisterAnimation(animation, key);
    return animation;
}

CAniElement* AnimationRegistry::LoadNamedAnimation(CRezItm* source) {
    if (source == NULL) {
        return NULL;
    }
    return LoadAnimationFromSource(source->GetName(), source);
}

i32 AnimationRegistry::LoadFromTree(CRezDir* tree, const std::string& prefix, const std::string& separator) {
    i32 loadedCount = 0;
    CRezDir* node = static_cast<CRezDir*>(tree->GetFirstSubDir());
    while (node != NULL) {
        const std::string keyBuffer = joinResourceKey(prefix, separator, node->GetDirName());
        loadedCount += LoadFromTree(node, keyBuffer, separator);
        node = static_cast<CRezDir*>(tree->GetNextSubDir(node));
    }
    CRezTyp* group = tree->GetFirstType();
    if (group != NULL) {
        do {

            CRezItm* source = tree->GetFirstItem(group);
            while (source != NULL) {
                if (source->GetType() == REZ_TAG_ANI) {
                    const std::string keyBuffer = joinResourceKey(prefix, separator, source->GetName());
                    if (LoadAnimationFromSource(keyBuffer, source) != NULL) {
                        ++loadedCount;
                    }
                }
                source = tree->GetNextItem(source);
            }
            group = tree->GetNextType(group);
        } while (group != NULL);
    }
    return loadedCount;
}

i32 AnimationRegistry::HasWithPrefix(const std::string& prefix) {
    i32 prefixLength = prefix.size();
    std::string key;
    CAniElement* animation = NULL;
    std::map<std::string, CAniElement*>::iterator pos = m_animations.begin();
    while (pos != m_animations.end()) {
        (key = pos->first, animation = pos->second, ++pos);
        if (key.compare(0, prefixLength, prefix) == 0) {
            return 1;
        }
    }
    return 0;
}

std::string AnimationRegistry::FindAnimationKey(CAniElement* target) {
    std::string key;
    if (target == NULL) {
        return key;
    }
    CAniElement* animation = NULL;
    std::map<std::string, CAniElement*>::iterator pos = m_animations.begin();
    while (pos != m_animations.end()) {
        (key = pos->first, animation = pos->second, ++pos);
        if (animation == target) {
            return key;
        }
    }
    (key).erase();
    return key;
}

CAniElement::~CAniElement() {
    DeleteAll();
}
