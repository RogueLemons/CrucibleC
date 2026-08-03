#pragma once

#include <clang/AST/Decl.h>

#include <string>
#include <unordered_map>

class StructDatabase {
public:
    enum class Kind {
        Free,
        Pod,
        Raii,
        Invalid
    };

    enum class FunctionKind {
        FreeCreator,
        PodCreator,
        RaiiCreator,
        Destroy,
        Copy,
        Move,
        Valid
    };

    struct StructInfo {
        const clang::RecordDecl *decl = nullptr;

        bool hasFreeCreator = false;
        bool hasPodCreator = false;
        bool hasRaiiCreator = false;

        bool hasDestroy = false;
        bool hasCopy = false;
        bool hasMove = false;
        bool hasValid = false;

        Kind kind = Kind::Invalid;
    };

private:
    std::unordered_map<std::string, StructInfo> structs;

public:
    StructInfo &registerStruct(const clang::RecordDecl *RD)
    {
        auto &info = structs[RD->getNameAsString()];
        info.decl = RD;
        return info;
    }

    StructInfo &registerFunction(
        const std::string &structName,
        FunctionKind kind)
    {
        auto &info = structs[structName];

        switch (kind) {
        case FunctionKind::FreeCreator:
            info.hasFreeCreator = true;
            break;

        case FunctionKind::PodCreator:
            info.hasPodCreator = true;
            break;

        case FunctionKind::RaiiCreator:
            info.hasRaiiCreator = true;
            break;

        case FunctionKind::Destroy:
            info.hasDestroy = true;
            break;

        case FunctionKind::Copy:
            info.hasCopy = true;
            break;

        case FunctionKind::Move:
            info.hasMove = true;
            break;

        case FunctionKind::Valid:
            info.hasValid = true;
            break;
        }

        return info;
    }

    void finalize()
    {
        for (auto &[name, info] : structs) {

            const int creators =
                static_cast<int>(info.hasFreeCreator) +
                static_cast<int>(info.hasPodCreator) +
                static_cast<int>(info.hasRaiiCreator);

            if (creators != 1) {
                info.kind = Kind::Invalid;
                continue;
            }

            if (info.hasFreeCreator) {
                info.kind = Kind::Free;
            }
            else if (info.hasPodCreator) {
                info.kind = Kind::Pod;
            }
            else {
                info.kind = Kind::Raii;
            }
        }
    }

    StructInfo *find(const std::string &name)
    {
        auto it = structs.find(name);

        if (it == structs.end())
            return nullptr;

        return &it->second;
    }

    const StructInfo *find(const std::string &name) const
    {
        auto it = structs.find(name);

        if (it == structs.end())
            return nullptr;

        return &it->second;
    }

    bool contains(const std::string &name) const
    {
        return structs.find(name) != structs.end();
    }

    const auto &allStructs() const
    {
        return structs;
    }

    void clear()
    {
        structs.clear();
    }
};