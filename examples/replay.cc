#include "sc2api/sc2_api.h"

#include "sc2utils/sc2_manage_process.h"

#include <iostream>
#include <queue>

using namespace sc2;

const char* kReplayFolder = "/home/jeffrey/MyReplays/";


/**
    1. Store the actions in OnStep
    2. Stop the replay after N iterations
    3. Start a multiplayer game.
    4. Pass the stored actions into the multiplayer game. 
    
    Multiplayer game is now ready for two of our agents. 
*/

class Replay : public sc2::ReplayObserver {
public:
    std::vector<uint32_t> count_units_built_;
    std::queue<RawActions> raw_actions_array;

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
        const sc2::ObservationInterface* obs = Observation();
        const RawActions& raw = obs->GetRawActions();

        for (int i = 0; i < raw.size(); i++) {
            printf("Ability_id %d. ", raw[i].ability_id);
            for (int j = 0; j < raw[i].unit_tags.size(); j++) {
                printf("Tags %d ", raw[i].unit_tags[j]);
            }
        }
        raw_actions_array.push(raw); // make sure don't need reserve()
        printf("ONSTEP LOOK AT ACTIONS %d \n", raw.size());
            
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

    RawActions GetNextRawActions() {
        RawActions ret = raw_actions_array.front();
        raw_actions_array.pop();
        return ret;
    }
};

class Bot : public Agent {
public: 
    virtual void OnGameStart() final {
        printf("Multiplayer game started");
    }
    virtual void OnStep() final {
        printf("Multiplayer game stepping");
    }
};

int main(int argc, char* argv[]) {
    int stop_iter = 100;
    int i = 0;
    sc2::Coordinator replay_coordinator;
    sc2::Coordinator multiagent_coordinator;
    Replay replay_observer;
    Bot bot; 

    printf("Starting replay \n ");
    if (!replay_coordinator.LoadSettings(argc, argv)) {
        return 1;
    }
    if (!replay_coordinator.SetReplayPath(kReplayFolder)) {
        printf("Replay Folder: %s \n", kReplayFolder);
        std::cout << "Unable to find replays." << std::endl;
        return 1;
    }
    replay_coordinator.AddReplayObserver(&replay_observer);

    while (i++ < stop_iter){
        replay_coordinator.Update();
    }
    replay_coordinator.LeaveGame();
    printf("Finished replay with %d ticks \n ", i);

    
    replay_coordinator.WaitForAllResponses();
    multiagent_coordinator.WaitForAllResponses();
    while (!sc2::PollKeyPress());


    printf("Starting multiplayer game \n ");
    if (!multiagent_coordinator.LoadSettings(argc, argv)) {
        return 1;
    }
    multiagent_coordinator.SetParticipants({
        CreateComputer(Race::Zerg),
        CreateParticipant(Race::Terran, &bot)
    });
    multiagent_coordinator.LaunchStarcraft();

    multiagent_coordinator.StartGame(sc2::kMapBelShirVestigeLE);
    i = 0;
    while (i++ < stop_iter){
        // pass in actions here 
        replay_coordinator.Update();
    }
}
