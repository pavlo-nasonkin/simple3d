#pragma once

#include <string>
#include <memory>

#include "TextureFilterBase.h"
#include "resources/Texture2D.h"

class TextureMapFilter : public TextureFilterBase
{
public:
    explicit TextureMapFilter(const std::shared_ptr<Texture2D>& texture);
    ~TextureMapFilter() override = default;

protected:
    const std::string& GetBaseFilterCode() const override {
        return _filterCode;
    }

private:
    static std::string _filterCode;
};