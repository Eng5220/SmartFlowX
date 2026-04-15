Set-StrictMode -Version 2
$ErrorActionPreference = "Stop"

git checkout -b rewrite-history main

Write-Output "Rewriting 6329d51..."
git cherry-pick 6329d51
git commit --amend -m "Implement core device interfaces" -m "Established core device interfaces within the hardware abstraction layer to encapsulate individual driver functionality, improving modularity and structural separation of concerns."

Write-Output "Rewriting b709b6c..."
git cherry-pick b709b6c
git commit --amend -m "Implement gesture-based monitoring system" -m "Integrated a gesture-based monitoring abstraction into the hardware layer, enabling event-driven responses to proximity triggers and directional inputs."

Write-Output "Rewriting be6fe3e..."
git cherry-pick be6fe3e
git commit --amend -m "Implement pump and gesture sensor drivers with monitoring" -m "Created hardware abstraction interfaces and corresponding drivers for the pump, flow meter, and gesture sensor. Included initial monitoring support structures to allow state observability."

Write-Output "Rewriting 6a4b9f8..."
git cherry-pick 6a4b9f8
git commit --amend -m "Implement core hardware device controllers" -m "Added initial device controllers aligned against the hardware abstraction interfaces, centralizing direct hardware I/O operations into dedicated class boundaries."

Write-Output "Rewriting 8459aa1..."
git cherry-pick 8459aa1
git commit --amend -m "Implement display and driver interfaces" -m "Established the foundational hardware abstraction layer by introducing interfaces and initial drivers for the flow meter, gesture sensor, and LCD display. This provides an abstraction boundary for consumers requiring sensory feedback."

Write-Output "Rewriting 52daae0..."
git cherry-pick 52daae0
git commit --amend -m "Decouple FillingController from concrete hardware drivers" -m "Added behavioral interfaces for the proximity sensor, pump, and flow meter, implementing them across their respective controllers. Updated FillingController to depend entirely on these abstractions rather than concrete classes, enabling safer hardware substitution and module isolation."
git branch -f feature/solid-refactor HEAD

Write-Output "Rewriting 646afba..."
git cherry-pick 646afba
git commit --amend -m "Update architectural documentation" -m "Revised the project checklist and underlying documentation to reflect recent structural modifications across the hardware abstraction layers and state machine controllers."

Write-Output "Rewriting ca99d51..."
git cherry-pick ca99d51
git commit --amend -m "Add substitution stress harness and proximity callback contract" -m "Introduced a new lsp_stress_test executable that exercises base interface substitutability across lifecycle and callback scenarios. Updated IHardwareDevice and IProximitySensor contracts to support multi-channel sensor payloads without breaking existing consumers, ensuring robust backwards compatibility."
git branch -f feature/sensor-callback-audit HEAD

Write-Output "Cherry-picking 8db4469 (leaving message intact)..."
git cherry-pick 8db4469

Write-Output "Rewriting 58edbc9..."
git cherry-pick 58edbc9
git commit --amend -m "Document global state audit completion" -m "Recorded the completion of the mutable global-variable audit. Documented the removal of mutable namespace-scope state and clarified that remaining static usages are safely constrained to constants and static methods, providing clear traceability for the architecture implementation."

Write-Output "Rewriting a163256..."
git cherry-pick a163256
git commit --amend -m "Enforce data encapsulation across payload structs" -m "Converted public data fields in GestureEvent and CallbackStats into private fields with explicit getter and setter methods. Updated all consumer logic in FillingController and test suites to consume the new getter methods instead of accessing properties directly, enforcing strict encapsulation mechanisms."

Write-Output "Rewriting 306eb2c..."
git cherry-pick 306eb2c
git commit --amend -m "Document data access encapsulation status" -m "Updated the architectural documentation to confirm that all custom C++ classes use proper accessor interfaces. Audited the codebase to ensure direct member access is completely eliminated from object-oriented components, with exemptions accurately noted for standard POSIX C structs used in kernel system calls."

Write-Output "Rewriting a118ec7..."
git cherry-pick a118ec7
git commit --amend -m "Eliminate heap allocations in GestureEvent callbacks" -m "Replaced the dynamic std::vector inside the GestureEvent payload with a custom stack-allocated bounded array class (InlineChannels) to achieve O(1) memory overhead. This guarantees zero heap allocations during high-frequency I/O cycles, optimizing the threaded data pathways."

Write-Output "Rewriting 3c913a9..."
git cherry-pick 3c913a9
git commit --amend -m "Clarify data structure efficiency status documentation" -m "Refined the project documentation to accurately describe the optimization of the GestureEvent payload. Adjusted the wording to 'per-event heap allocations in user-space payload handling' to correctly reflect the user-space nature of the overhead and avoid implications of kernel allocations."

Write-Output "Rewriting 9caa522..."
git cherry-pick 9caa522
git commit --amend -m "Extract orchestration logic from main to AquaFlowApp" -m "Introduced a dedicated AquaFlowApp class to encapsulate dependency injection, hardware callback bindings, and the timer tick loop. Stripped all real-time processing, LCD rendering logic, and shutdown sequences out of main.cpp, ensuring the entry point is strictly limited to initialization and signal blocking."
git branch -f refactor/enforce-encapsulation-rules HEAD

Write-Output "Cleaning up branch..."
git checkout refactor/enforce-encapsulation-rules
git branch -D rewrite-history

Write-Output "Done! All branches updated."
