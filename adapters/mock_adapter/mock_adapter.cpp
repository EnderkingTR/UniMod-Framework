#include "mock_adapter.hpp"
#include <unimod/core/logger.hpp>
#include <imgui.h>

namespace unimod::adapters::mock {

MockAdapter::MockAdapter() {
    UNIMOD_LOG_INFO("MockAdapter instance constructed.");
}

MockAdapter::~MockAdapter() {
    OnDetach();
}

bool MockAdapter::OnDetectEngine() {
    // Probe simulated proprietary game context
    if (m_Context.signature == 0xDEADBEEF) {
        UNIMOD_LOG_INFO("MockAdapter: Detected target proprietary engine context (Signature: 0x{:08X}).", m_Context.signature);
        return true;
    }
    return false;
}

bool MockAdapter::OnAttach() {
    if (m_Attached) return true;

    UNIMOD_LOG_INFO("MockAdapter: OnAttach initiated. Hooking simulated engine loops...");
    m_Attached = true;
    UNIMOD_LOG_INFO("MockAdapter: Successfully attached. Player: '{}', Health: {}/{}",
        m_Context.playerName, m_Context.playerHealth, m_Context.maxHealth);
    return true;
}

void MockAdapter::OnInitRenderer() {
    m_RendererReady = true;
    UNIMOD_LOG_INFO("MockAdapter: Renderer initialized, ImGui widgets ready.");
}

void MockAdapter::OnUpdate(float deltaTime) {
    if (!m_Attached) return;

    m_Context.tickCount++;

    // Apply active cheats / engine manipulations
    if (m_Context.godMode) {
        m_Context.playerHealth = m_Context.maxHealth;
    }
}

void MockAdapter::OnRenderOverlay() {
    if (!m_Attached) return;

    ImGui::SetNextWindowSize(ImVec2(450, 320), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Mock Engine Inspector - Live Object Traversal", nullptr, ImGuiWindowFlags_None)) {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Target Engine: Proprietary Game Engine v1.0");
        ImGui::Separator();

        ImGui::Text("Engine Context Address: %p", static_cast<void*>(&m_Context));
        ImGui::Text("Simulation Ticks: %llu", static_cast<unsigned long long>(m_Context.tickCount));
        ImGui::Spacing();

        ImGui::SeparatorText("Player Entity Attributes");
        ImGui::InputText("Character Name", m_Context.playerName, sizeof(m_Context.playerName));

        ImGui::SliderInt("Health Points", &m_Context.playerHealth, 0, m_Context.maxHealth);
        ImGui::SliderFloat("Move Speed", &m_Context.moveSpeed, 1.0f, 30.0f, "%.1f m/s");

        ImGui::Checkbox("God Mode (Invulnerable)", &m_Context.godMode);
        ImGui::SameLine();
        if (ImGui::Button("Heal to Max")) {
            m_Context.playerHealth = m_Context.maxHealth;
            UNIMOD_LOG_INFO("MockAdapter: Player health manually restored to max via ImGui!");
        }
    }
    ImGui::End();
}

void MockAdapter::OnDetach() {
    if (m_Attached) {
        UNIMOD_LOG_INFO("MockAdapter: OnDetach called. Releasing engine hooks and simulated context.");
        m_Attached = false;
        m_RendererReady = false;
    }
}

} // namespace unimod::adapters::mock
