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
        Return,
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
        bool hasReturn = false;
        bool hasValid = false;

        Kind kind = Kind::Invalid;
    };

private:
    std::unordered_map<std::string, StructInfo> structs;

public:
    StructInfo &registerStruct(const clang::RecordDecl *RD);

    StructInfo &registerFunction(
        const std::string &structName,
        FunctionKind kind);

    void finalize();

    StructInfo *find(const std::string &name);

    const StructInfo *find(const std::string &name) const;

    bool contains(const std::string &name) const;

    const std::unordered_map<std::string, StructInfo> &allStructs() const;

    void clear();
};
