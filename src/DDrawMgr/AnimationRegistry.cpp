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
            m_animations.erase((key).c_str());
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

i32 AnimationRegistry::RemoveWithPrefix(const char* prefix, const char* separator) {
    std::string match(prefix);
    match += separator;
    i32 prefixLength = static_cast<i32>((match).size());
    std::string key;
    CAniElement* animation = NULL;
    std::map<std::string, CAniElement*>::iterator pos = m_animations.begin();
    i32 removedCount = 0;
    while (pos != m_animations.end()) {
        (key = pos->first, animation = pos->second, ++pos);
        if (strncmp((key).c_str(), (match).c_str(), prefixLength) == 0) {
            m_animations.erase((key).c_str());
            if (animation != NULL) {
                delete animation;
            }
            ++removedCount;
        }
    }
    return removedCount;
}

CAniElement* AnimationRegistry::LoadAnimationFromSource(const char* key, CRezItm* source) {
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

CAniElement* AnimationRegistry::LoadAnimationFromFile(const char* key, const char* path) {
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

void AnimationRegistry::AddAnimation(CAniElement* animation, const char* key) {
    RegisterAnimation(animation, key);
}

i32 AnimationRegistry::LoadFromTree(CRezDir* tree, const char* prefix, const char* separator) {
    i32 loadedCount = 0;
    char* keyBuffer = new char[0x100];
    if (keyBuffer == NULL) {
        return 0;
    }
    keyBuffer[0] = 0;
    CRezDir* node = static_cast<CRezDir*>(tree->GetFirstSubDir());
    while (node != NULL) {
        if (prefix != NULL && *prefix != 0) {
            sprintf(keyBuffer, "%s%s%s", prefix, separator, node->GetDirName());
        } else {
            strcpy(keyBuffer, node->GetDirName());
        }
        loadedCount += LoadFromTree(node, keyBuffer, separator);
        node = static_cast<CRezDir*>(tree->GetNextSubDir(node));
    }
    CRezTyp* group = tree->GetFirstType();
    if (group != NULL) {
        do {

            CRezItm* source = tree->GetFirstItem(group);
            while (source != NULL) {
                if (source->GetType() == REZ_TAG_ANI) {
                    if (prefix != NULL && *prefix != 0) {
                        sprintf(keyBuffer, "%s%s%s", prefix, separator, source->GetName());
                    } else {
                        strcpy(keyBuffer, source->GetName());
                    }
                    if (LoadAnimationFromSource(keyBuffer, source) != NULL) {
                        ++loadedCount;
                    }
                }
                source = tree->GetNextItem(source);
            }
            group = tree->GetNextType(group);
        } while (group != NULL);
    }
    delete[] keyBuffer;
    return loadedCount;
}

i32 AnimationRegistry::HasWithPrefix(const char* prefix) {
    i32 prefixLength = strlen(prefix);
    std::string key;
    CAniElement* animation = NULL;
    std::map<std::string, CAniElement*>::iterator pos = m_animations.begin();
    while (pos != m_animations.end()) {
        (key = pos->first, animation = pos->second, ++pos);
        if (strncmp((key).c_str(), prefix, prefixLength) == 0) {
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
