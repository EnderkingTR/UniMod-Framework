#include <iostream>
#include <memory>
#include <cassert>

#include <unimod/core/logger.hpp>
#include <unimod/adapters/adapter_manager.hpp>
#include "../../adapters/mock_adapter/mock_adapter.hpp"

int main() {
    std::cout << "====================================================\n";
    std::cout << "  UniMod Framework - Phase 4 Verification Test\n";
    std::cout << "====================================================\n";

    unimod::core::Logger::Initialize("phase4_test.log", false);

    auto& manager = unimod::adapters::AdapterManager::GetInstance();

    std::cout << "\n[Step 1] Verifying initial AdapterManager state...\n";
    if (manager.GetRegisteredCount() != 0 || manager.GetActiveAdapter() != nullptr) {
        std::cerr << "FAIL: AdapterManager not initially empty!\n";
        return 1;
    }
    std::cout << "-> AdapterManager is initially clean.\n";

    std::cout << "\n[Step 2] Registering MockAdapter...\n";
    auto mockAdapter = std::make_unique<unimod::adapters::mock::MockAdapter>();
    auto* pRawMock = mockAdapter.get();
    manager.RegisterAdapter(std::move(mockAdapter));

    if (manager.GetRegisteredCount() != 1) {
        std::cerr << "FAIL: Registered count mismatch! Expected 1, got " << manager.GetRegisteredCount() << "\n";
        return 1;
    }
    std::cout << "-> Registered adapter: '" << pRawMock->GetName() << "' (v" << pRawMock->GetVersion() << ").\n";

    std::cout << "\n[Step 3] Running DetectAndActivateAdapter()...\n";
    bool activated = manager.DetectAndActivateAdapter();
    if (!activated) {
        std::cerr << "FAIL: Engine detection or attachment failed!\n";
        return 1;
    }

    if (manager.GetActiveAdapter() != pRawMock || !pRawMock->IsAttached()) {
        std::cerr << "FAIL: Active adapter pointer mismatch or attachment state false!\n";
        return 1;
    }
    std::cout << "-> Adapter successfully attached and active.\n";

    std::cout << "\n[Step 4] Verifying frame update tick dispatch...\n";
    auto* ctx = pRawMock->GetContext();
    for (int i = 0; i < 15; ++i) {
        manager.Update(0.016f);
    }
    std::cout << "-> Dispatched 15 ticks. Context tick count: " << ctx->tickCount << " (Expected: 15)\n";
    if (ctx->tickCount != 15) {
        std::cerr << "FAIL: Tick count does not match dispatched frames!\n";
        return 1;
    }

    std::cout << "\n[Step 5] Testing memory manipulation and God Mode logic...\n";
    ctx->playerHealth = 25;
    std::cout << "-> Reduced player health to: " << ctx->playerHealth << "\n";
    manager.Update(0.016f);
    if (ctx->playerHealth != 25) {
        std::cerr << "FAIL: Health changed unexpectedly without God Mode!\n";
        return 1;
    }

    std::cout << "-> Activating God Mode...\n";
    ctx->godMode = true;
    manager.Update(0.016f); // Update tick should immediately heal player to maxHealth
    std::cout << "-> Post-tick player health: " << ctx->playerHealth << " (Expected: " << ctx->maxHealth << ")\n";
    if (ctx->playerHealth != ctx->maxHealth) {
        std::cerr << "FAIL: God Mode did not restore health to max!\n";
        return 1;
    }

    std::cout << "\n[Step 6] Testing clean adapter shutdown & detachment...\n";
    manager.Shutdown();
    if (manager.GetActiveAdapter() != nullptr || pRawMock->IsAttached() || manager.GetRegisteredCount() != 0) {
        std::cerr << "FAIL: Cleanup failed! Adapter still registered or active.\n";
        return 1;
    }
    std::cout << "-> Adapter safely detached, hooks released, registry cleaned.\n";

    unimod::core::Logger::Shutdown();

    std::cout << "\n====================================================\n";
    std::cout << "[SUCCESS] Phase 4 verification passed completely!\n";
    std::cout << "====================================================\n";
    return 0;
}
