#include "struct_database.hpp"

StructDatabase::StructInfo &StructDatabase::registerStruct(const clang::RecordDecl *RD)
{
    auto &info = structs[RD->getNameAsString()];
    info.decl = RD;
    return info;
}

StructDatabase::StructInfo &StructDatabase::registerFunction(
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

    case FunctionKind::Return:
        info.hasReturn = true;
        break;

    case FunctionKind::Valid:
        info.hasValid = true;
        break;
    }

    return info;
}

void StructDatabase::finalize()
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

StructDatabase::StructInfo *StructDatabase::find(const std::string &name)
{
    auto it = structs.find(name);

    if (it == structs.end())
        return nullptr;

    return &it->second;
}

const StructDatabase::StructInfo *StructDatabase::find(const std::string &name) const
{
    auto it = structs.find(name);

    if (it == structs.end())
        return nullptr;

    return &it->second;
}

bool StructDatabase::contains(const std::string &name) const
{
    return structs.find(name) != structs.end();
}

const std::unordered_map<std::string, StructDatabase::StructInfo> &StructDatabase::allStructs() const
{
    return structs;
}

void StructDatabase::clear()
{
    structs.clear();
}
