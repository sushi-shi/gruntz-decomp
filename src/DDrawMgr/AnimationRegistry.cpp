#include <rva.h>

#include <Mfc.h>

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

RVA(0x00152640, 0x6)
i32 AnimationRegistry::IsReady() {
    return 1;
}
RVA(0x00152650, 0x5)
void AnimationRegistry::Unload() {
    ClearAnimations();
}

// @early-stop
RVA(0x00152660, 0xb2)
void AnimationRegistry::RemoveAnimation(CAniElement* target) {
    if (target == NULL) {
        return;
    }
    POSITION pos = m_animations.GetStartPosition();
    CString key;
    CAniElement* animation = NULL;
    while (pos != NULL) {
        MapGetNext(m_animations, pos, key, animation);
        if (target == animation) {
            m_animations.RemoveKey(key);
            delete target;
            break;
        }
    }
}

RVA(0x00152720, 0xa2)
void AnimationRegistry::ClearAnimations() {
    POSITION pos = m_animations.GetStartPosition();
    CString key;
    CAniElement* animation = NULL;
    if (pos != NULL) {
        do {
            MapGetNext(m_animations, pos, key, animation);
            if (animation != NULL) {
                delete animation;
            }
        } while (pos != NULL);
    }
    m_animations.RemoveAll();
}

RVA(0x001527d0, 0xf8)
i32 AnimationRegistry::RemoveWithPrefix(const char* prefix, const char* separator) {
    CString match(prefix);
    match += separator;
    i32 prefixLength = match.GetLength();
    CString key;
    CAniElement* animation = NULL;
    POSITION pos = m_animations.GetStartPosition();
    i32 removedCount = 0;
    while (pos != NULL) {
        MapGetNext(m_animations, pos, key, animation);
        if (strncmp(key, match, prefixLength) == 0) {
            m_animations.RemoveKey(key);
            if (animation != NULL) {
                delete animation;
            }
            ++removedCount;
        }
    }
    return removedCount;
}

#define REGISTER_ANIMATION(animation, key) m_animations[key] = animation

RVA(0x001528d0, 0xdd)
CAniElement* AnimationRegistry::LoadAnimationFromSource(const char* key, CRezArchiveEntry* source) {
    CAniElement* animation = new CAniElement;
    if (animation == NULL) {
        return NULL;
    }
    if (animation->Configure(OwnerMgr()->m_soundRegistry, source, 0) == 0) {

        delete animation;
        return NULL;
    }
    REGISTER_ANIMATION(animation, key);
    return animation;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001529b0, 0xdd)
CAniElement* AnimationRegistry::LoadAnimationFromFile(const char* key, const char* path) {
    CAniElement* animation = new CAniElement;
    if (animation == NULL) {
        return NULL;
    }
    if (animation->LoadFile(OwnerMgr()->m_soundRegistry, path, 0) == 0) {

        delete animation;
        return NULL;
    }
    REGISTER_ANIMATION(animation, key);
    return animation;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00152a90, 0x17)
CAniElement* AnimationRegistry::LoadNamedAnimation(CRezArchiveEntry* source) {
    if (source == NULL) {
        return NULL;
    }
    return LoadAnimationFromSource(source->m_name, source);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00152ab0, 0x16)
void AnimationRegistry::AddAnimation(CAniElement* animation, const char* key) {
    REGISTER_ANIMATION(animation, key);
}

RVA(0x00152ad0, 0x17f)
i32 AnimationRegistry::LoadFromTree(
    CRezArchiveDir* tree,
    const char* prefix,
    const char* separator
) {
    i32 loadedCount = 0;
    char* keyBuffer = new char[0x100];
    if (keyBuffer == NULL) {
        return 0;
    }
    keyBuffer[0] = 0;
    CRezArchiveDir* node = static_cast<CRezArchiveDir*>(tree->FirstSubdirectory());
    while (node != NULL) {
        if (prefix != NULL && *prefix != 0) {
            sprintf(keyBuffer, "%s%s%s", prefix, separator, node->m_name);
        } else {
            strcpy(keyBuffer, node->m_name);
        }
        loadedCount += LoadFromTree(node, keyBuffer, separator);
        node = static_cast<CRezArchiveDir*>(tree->NextSubdirectory(node));
    }
    CRezArchiveType* group = tree->FirstType();
    if (group != NULL) {
        do {

            CRezArchiveEntry* source = tree->FirstEntry(group);
            while (source != NULL) {
                if (source->GetTypeTag() == REZ_TAG_ANI) {
                    if (prefix != NULL && *prefix != 0) {
                        sprintf(keyBuffer, "%s%s%s", prefix, separator, source->m_name);
                    } else {
                        strcpy(keyBuffer, source->m_name);
                    }
                    if (LoadAnimationFromSource(keyBuffer, source) != NULL) {
                        ++loadedCount;
                    }
                }
                source = tree->NextEntry(source);
            }
            group = tree->NextType(group);
        } while (group != NULL);
    }
    delete[] keyBuffer;
    return loadedCount;
}

RVA(0x00152c50, 0xdc)
i32 AnimationRegistry::HasWithPrefix(const char* prefix) {
    i32 prefixLength = strlen(prefix);
    CString key;
    CAniElement* animation = NULL;
    POSITION pos = m_animations.GetStartPosition();
    while (pos != NULL) {
        MapGetNext(m_animations, pos, key, animation);
        if (strncmp(key, prefix, prefixLength) == 0) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00152d30, 0xd4)
CString AnimationRegistry::FindAnimationKey(CAniElement* target) {
    CString key;
    if (target == NULL) {
        return key;
    }
    CAniElement* animation = NULL;
    POSITION pos = m_animations.GetStartPosition();
    while (pos != NULL) {
        MapGetNext(m_animations, pos, key, animation);
        if (animation == target) {
            return key;
        }
    }
    key.Empty();
    return key;
}

RVA_COMPGEN(0x00152e10, 0x1e, ??_GCAniElement@@UAEPAXI@Z)
RVA(0x00152e30, 0x53)
CAniElement::~CAniElement() {
    DeleteAll();
}
