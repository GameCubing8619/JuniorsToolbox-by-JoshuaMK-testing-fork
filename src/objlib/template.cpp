#include "objlib/template.hpp"
#include "color.hpp"
#include "jsonlib.hpp"
#include "magic_enum.hpp"
#include "objlib/meta/enum.hpp"
#include "objlib/meta/member.hpp"
#include "objlib/meta/struct.hpp"
#include "objlib/transform.hpp"
#include <expected>
#include <glm/glm.hpp>
#include <optional>

using json = nlohmann::json;

namespace Toolbox::Object {

    std::expected<std::vector<MetaMember>, SerialError>
    Template::getMembers(std::string_view wizard) const {
        return std::expected<std::vector<MetaMember>, SerialError>();
    }

    std::expected<void, SerialError> Template::serialize(Serializer &out) const { return {}; }

    std::expected<void, SerialError> Template::deserialize(Deserializer &in) {
        json template_json;

        auto result = tryJSON(template_json, [&](json &j) {
            in.stream() >> j;

            m_name = j.at(0).get<std::string>();

            json &metadata    = j[m_name];
            json &member_data = metadata["Members"];
            json &struct_data = metadata["Structs"];
            json &enum_data   = metadata["Enums"];

            TemplateWizard default_wizard;

            cacheEnums(enum_data);
            cacheStructs(struct_data);
            loadMembers(member_data, default_wizard.m_init_members);

            m_wizards.push_back(default_wizard);

            loadWizards(metadata["Wizard"]);
        });

        if (!result) {
            JSONError &err = result.error();
            return std::unexpected(
                make_serial_error(err.m_message, err.m_reason, err.m_byte, in.filepath()));
        }

        return {};
    }

    void Template::cacheEnums(json &enums) {
        for (const auto &item : enums.items()) {
            const auto &e  = item.value();
            auto enum_type = magic_enum::enum_cast<MetaType>(e["type"].get<std::string>());
            if (!enum_type.has_value()) {
                continue;
            }
            bool enum_bitmask = e["Multi"].get<bool>();
            std::vector<MetaEnum::enum_type> values;
            for (auto &value : e["Flags"].items()) {
                auto vs                   = value.value().get<std::string>();
                MetaEnum::enum_type enumv = {value.key(), MetaValue(std::stoi(vs, nullptr, 0))};
                values.push_back(enumv);
            }
            MetaEnum menum(item.key(), enum_type.value(), values, enum_bitmask);
            m_enum_cache.emplace_back(menum);
        }
    }

    void Template::cacheStructs(json &structs) {
        std::vector<std::string> visited;
        for (const auto &item : structs.items()) {
            auto name         = item.key();
            auto json_members = item.value();

            std::vector<MetaMember> members;
            loadMembers(json_members, members);

            MetaStruct mstruct(item.key(), members);
            m_struct_cache.emplace_back(mstruct);
        }
    }

    std::optional<MetaMember> Template::loadMemberEnum(std::string_view name, std::string_view type,
                                                       MetaMember::size_type array_size) {
        auto enum_ = std::find_if(m_enum_cache.begin(), m_enum_cache.end(),
                                  [&](const auto &e) { return e.name() == type; });
        if (enum_ == m_enum_cache.end()) {
            return {};
        }
        std::vector<MetaEnum> enums;

        size_t asize = 1;
        if (std::holds_alternative<std::shared_ptr<MetaValue>>(array_size)) {
            asize = std::get<std::shared_ptr<MetaValue>>(array_size)->get<size_t>().value();
        } else {
            asize = std::get<size_t>(array_size);
        }

        enums.reserve(asize);
        for (size_t i = 0; i < asize; ++i) {
            enums.emplace_back(*enum_);
        }
        return MetaMember(name, enums);
    }

    std::optional<MetaMember> Template::loadMemberStruct(std::string_view name,
                                                         std::string_view type,
                                                         MetaMember::size_type array_size) {
        auto struct_ = std::find_if(m_struct_cache.begin(), m_struct_cache.end(),
                                    [&](const auto &e) { return e.name() == type; });
        if (struct_ == m_struct_cache.end()) {
            return {};
        }
        std::vector<MetaStruct> structs;

        size_t asize = 1;
        if (std::holds_alternative<std::shared_ptr<MetaValue>>(array_size)) {
            asize = std::get<std::shared_ptr<MetaValue>>(array_size)->get<size_t>().value();
        } else {
            asize = std::get<size_t>(array_size);
        }

        structs.reserve(asize);

        for (size_t i = 0; i < asize; ++i) {
            structs.emplace_back(*struct_);
        }
        return MetaMember(name, structs);
    }

    std::optional<MetaMember> Template::loadMemberPrimitive(std::string_view name,
                                                            std::string_view type,
                                                            MetaMember::size_type array_size) {
        auto vtype = magic_enum::enum_cast<MetaType>(type);
        if (!vtype)
            return {};

        std::vector<MetaValue> values;

        size_t asize = 1;
        if (std::holds_alternative<std::shared_ptr<MetaValue>>(array_size)) {
            asize = std::get<std::shared_ptr<MetaValue>>(array_size)->get<size_t>().value();
        } else {
            asize = std::get<size_t>(array_size);
        }

        values.reserve(asize);

        for (size_t i = 0; i < asize; ++i) {
            switch (vtype.value()) {
            case MetaType::BOOL:
                values.emplace_back(MetaValue(false));
                break;
            case MetaType::S8:
                values.emplace_back(MetaValue(static_cast<s8>(0)));
                break;
            case MetaType::U8:
                values.emplace_back(MetaValue(static_cast<u8>(0)));
                break;
            case MetaType::S16:
                values.emplace_back(MetaValue(static_cast<s16>(0)));
                break;
            case MetaType::U16:
                values.emplace_back(MetaValue(static_cast<u16>(0)));
                break;
            case MetaType::S32:
                values.emplace_back(MetaValue(static_cast<s32>(0)));
                break;
            case MetaType::U32:
                values.emplace_back(MetaValue(static_cast<u32>(0)));
                break;
            case MetaType::F32:
                values.emplace_back(MetaValue(static_cast<f32>(0)));
                break;
            case MetaType::F64:
                values.emplace_back(MetaValue(static_cast<f64>(0)));
                break;
            case MetaType::STRING:
                values.emplace_back(MetaValue(std::string()));
                break;
            case MetaType::VEC3:
                values.emplace_back(glm::vec3());
                break;
            case MetaType::TRANSFORM:
                values.emplace_back(Transform());
                break;
            case MetaType::RGB:
                values.emplace_back(Color::RGB24());
                break;
            case MetaType::RGBA:
                values.emplace_back(Color::RGBA32());
                break;
            default:
                // Unsupported type
                return {};
            }
        }
        return MetaMember(name, values);
    }

    void Template::loadMembers(json &members, std::vector<MetaMember> out) {
        for (const auto &item : members.items()) {
            auto member_name = item.key();
            auto member_info = item.value();

            auto member_type = member_info["type"].get<std::string>();
            MetaMember::size_type member_size;

            auto member_size_info = member_info["size"];
            if (member_size_info.is_number()) {
                member_size = member_size_info.get<size_t>();
            } else {
                auto member_size_str = member_size_info.get<std::string>();
                auto member_it       = std::find_if(out.begin(), out.end(), [&](const auto &e) {
                    return e.name() == member_size_str;
                });
                if (member_it != out.end()) {
                    auto value = member_it->value<MetaValue>(0);
                    if (value) {
                        member_size = value.value();
                    }
                }
            }

            auto member_type_enum = magic_enum::enum_cast<MetaType>(member_type);
            if (!member_type_enum.has_value()) {
                // This is a struct or enum
                {
                    auto enum_it =
                        std::find_if(m_enum_cache.begin(), m_enum_cache.end(),
                                     [&](const auto &e) { return e.name() == member_type; });
                    if (enum_it != m_enum_cache.end()) {
                        auto member = loadMemberEnum(member_name, member_type, member_size);
                        if (member) {
                            out.push_back(member.value());
                        }
                        continue;
                    }
                }

                {
                    auto struct_it =
                        std::find_if(m_struct_cache.begin(), m_struct_cache.end(),
                                     [&](const auto &e) { return e.name() == member_type; });
                    if (struct_it != m_struct_cache.end()) {
                        auto member = loadMemberStruct(member_name, member_type, member_size);
                        if (member) {
                            out.push_back(member.value());
                        }
                        continue;
                    }
                }
            } else {
                auto member = loadMemberPrimitive(member_name, member_type, member_size);
                if (member) {
                    out.push_back(member.value());
                }
            }
        }
    }

    void Template::loadWizards(json &wizards) {
        for (const auto &item : wizards.items()) {
            TemplateWizard wizard;
            wizard.m_name = item.key();
            for (const auto &mi : item.value()["members"].items()) {
                /* TODO: Properly initialize these according to members */
                wizard.m_init_members.emplace_back(MetaMember(mi.key(), MetaValue(0)));
            }
            m_wizards.emplace_back(wizard);
        }
    }

}  // namespace Toolbox::Object