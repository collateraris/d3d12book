#pragma once

#include <string>
#include <cassert>
#include <sstream>
#include <vector>
#include <memory>
#include <stdlib.h>

#include <config_sys/tinyxml2.h>

namespace txml = tinyxml2;

namespace dx12demo::core
{
    class XPath
    {
        public:
            static constexpr char PATH_DELIMITER = ':';

            XPath(const txml::XMLElement* root) : root(root) { assert(root != nullptr); };

            const txml::XMLElement* GetElement() const { return root; }

            std::string GetName() const { return root->Name(); }

            const char* GetText() const { return root->GetText(); }

            template<typename T>
            T GetValueText();

            XPath GetPath(const std::string& path) const { return XPath(GetElement(root, path)); }

            template<typename... Targs>
            XPath GetPath(const std::string& path, const Targs& ... args) const { return XPath(GetElement(root, path, args...)); }

            template<typename T>
            T GetAttribute(const std::string& name) const;

            template<typename T>
            void SetAttribute(const std::string& name, const T& val) const;

            template<typename T>
            XPath* FindChildByAttribute(const std::string& name, const T& val) const;

            std::vector<XPath> GetChildren() const;

        private:
            const txml::XMLElement* root;

            const txml::XMLElement* GetElement(const txml::XMLElement* elem, const std::string& path) const;

            template<typename... Targs>
            const txml::XMLElement* GetElement(const txml::XMLElement* elem, const std::string& path, const Targs& ... args) const
            {
                return GetElement(GetElement(elem, path), args...);
            }

            template<typename T>
            T TextValue(const std::string& text);

            template<>
            float TextValue(const std::string& text);

            template<>
            std::wstring TextValue(const std::string& text);
        };

        // ------------------------------------------------------------------ //

        template<typename T>
        T XPath::TextValue(const std::string& text)
        {
            std::stringstream ss(text);
            T val;
            ss >> val;
            return val;
        }

        template<>
        float XPath::TextValue(const std::string& text)
        {
            float val = ::atof(text.c_str());
            return val;
        }

        template<>
        std::wstring XPath::TextValue(const std::string& text)
        {
            std::wstring val(text.cbegin(), text.cend());
            return val;
        }

        template<typename T>
        T XPath::GetValueText()
        {
            std::string text = root->GetText();
            return TextValue<T>(text);
        }

        template<typename T>
        T XPath::GetAttribute(const std::string& name) const
        {
            const txml::XMLAttribute* attr = root->FindAttribute(name.c_str());
            std::string text = "";

            assert(attr != nullptr);
            if (attr != nullptr)
                text = attr->Value();

            return TextValue<T>(text);
        };


        template<typename T>
        void XPath::SetAttribute(const std::string& name, const T& val) const
        {
            const txml::XMLAttribute* attr = root->FindAttribute(name.c_str());
            std::string text = "";

            std::stringstream ss;
            ss << val;
            text = ss.str();

            assert(attr != nullptr);
            if (attr != nullptr)
                attr->SetAttribute(text);
        };

        template<typename T>
        XPath* XPath::FindChildByAttribute(const std::string& name, const T& val) const
        {
            std::string text = "";
            std::stringstream ss;
            ss << val;
            text = ss.str();

            const txml::XMLElement* child = root->FirstChildElement();

            while (child != nullptr)
            {
                const txml::XMLAttribute* attr = child->FindAttribute(name.c_str());

                if (attr != nullptr && attr->Value() == text)
                    return new XPath(child);

                child = child->NextSiblingElement();
            }

            return nullptr;
        }
}