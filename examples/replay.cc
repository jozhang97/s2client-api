#include "sc2api/sc2_api.h"

#include "sc2utils/sc2_manage_process.h"

#include <iostream>

using namespace sc2;

const char* kReplayFolder = "/home/jeffrey/MyReplays/";

class Replay : public sc2::ReplayObserver {
public:
    std::vector<uint32_t> count_units_built_;

    Replay() :
        sc2::ReplayObserver() {
    }

    void OnGameStart() final {
        const sc2::ObservationInterface* obs = Observation();
        assert(obs->GetUnitTypeData().size() > 0);
        count_units_built_.resize(obs->GetUnitTypeData().size());
        std::fill(count_units_built_.begin(), count_units_built_.end(), 0);
    }
    
    void OnUnitCreated(const sc2::Unit& unit) final {
        assert(uint32_t(unit.unit_type) < count_units_built_.size());
        ++count_units_built_[unit.unit_type];
    }

    void OnStep() final {
        printf("STEP");
    }

    void OnGameEnd() final {
        std::cout << "Units created:" << std::endl;
        const sc2::ObservationInterface* obs = Observation();
        const sc2::UnitTypes& unit_types = obs->GetUnitTypeData();
        for (uint32_t i = 0; i < count_units_built_.size(); ++i) {
            if (count_units_built_[i] == 0) {
                continue;
            }

            std::cout << unit_types[i].name << ": " << std::to_string(count_units_built_[i]) << std::endl;
        }
        std::cout << "Finished" << std::endl;
    }
};


int main(int argc, char* argv[]) {
    sc2::Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv)) {
        return 1;
    }


    if (!coordinator.SetReplayPath(kReplayFolder)) {
        printf("%s \n", kReplayFolder);
        std::cout << "Unable to find replays." << std::endl;
        return 1;
    }

    Replay replay_observer;

    coordinator.AddReplayObserver(&replay_observer);

    coordinator.SetParticipants({
        CreateComputer(Race::Zerg),
        CreateComputer(Race::Terran)
    });

    coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    int i = 0;
    while (i++ < 100){
        printf("HERE");
        coordinator.Update();
    }
    while (!sc2::PollKeyPress());
}
