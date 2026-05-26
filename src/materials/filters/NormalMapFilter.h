#pragma once

#include <string>
#include <memory>

#include "TextureFilterBase.h"
#include "resources/Texture2D.h"

class NormalMapFilter : public TextureFilterBase
{
public:
    explicit NormalMapFilter(const std::shared_ptr<Texture2D>& texture);
    ~NormalMapFilter() override = default;

protected:
    const std::string& GetBaseFilterCode() const override {
        return _filterCode;
    }

private:
    static std::string _filterCode;
};