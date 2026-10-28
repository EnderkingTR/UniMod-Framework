#pragma once

#include <unimod/adapters/i_engine_adapter.hpp>
#include <cstdint>
#include <cstring>
#include <string>

namespace unimod::adapters::mock {

struct MockGameContext {
    uint32_t signature{0xDEADBEEF};
    int32_t playerHealth{100};
    int32_t maxHealth{100};
    float moveSpeed{6.5f};
    bool godMode{false};
    uint64_t tickCount{0};
    char playerName[32]{"HeroPlayer"};
};

class MockAdapter : public IEngineAdapter {
public:
    MockAdapter();
    virtual ~MockAdapter() override;

    virtual const char* GetName() const noexcept override {
        return "UniMod Mock Proprietary Engine Adapter";
    }

    virtual const char* GetVersion() const noexcept override {
        return "1.0.0";
    }

    virtual bool OnDetectEngine() override;
    virtual bool OnAttach() override;
    virtual void OnInitRenderer() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRenderOverlay() override;
    virtual void OnDetach() override;

    MockGameContext* GetContext() noexcept { return &m_Context; }
    bool IsAttached() const noexcept { return m_Attached; }

private:
    MockGameContext m_Context;
    bool m_Attached{false};
    bool m_RendererReady{false};
};

} // namespace unimod::adapters::mock
