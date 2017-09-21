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
        if (raw.size() != 0) 
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
    
    bool RawActionsEmpty() {
        return raw_actions_array.empty();
    }
            

};

class Bot : public Agent {
public: 
    virtual void OnGameStart() final {
        printf("Multiplayer game started \n");
    }
    virtual void OnStep() final {
        printf("Multiplayer game stepping \n");
    }
};

int main(int argc, char* argv[]) {
    int stop_iter = 100;
    int i = 0;
    sc2::Coordinator replay_coordinator;
    sc2::Coordinator multiagent_coordinator;
    Replay replay_observer;
    Bot bot; 
    Bot bot1; 
    RawActions raw;

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
    replay_coordinator.LeaveGame(); // doesn't do anything since no agents
    printf("Finished replay with %d ticks \n ", i);

    
    replay_coordinator.WaitForAllResponses();
    multiagent_coordinator.WaitForAllResponses();
    while (!sc2::PollKeyPress());


    printf("Starting multiplayer game \n ");
    if (!multiagent_coordinator.LoadSettings(argc, argv)) {
        return 1;
    }
    multiagent_coordinator.SetParticipants({
        CreateParticipant(Race::Terran, &bot),
        CreateParticipant(Race::Terran, &bot1)
    });
    multiagent_coordinator.LaunchStarcraft();

    multiagent_coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    i = 0;
    while (i++ < stop_iter) {
        printf("Iter: %d \n", i);
        replay_coordinator.Update();
    }
}

void run_raw_actions(RawActions* raw, ActionInterface* action_interface) {
    int action_index = 0;
    int tag_index = 0;
    ActionRaw action; 
    AbilityID ability_id; 
    std::vector<Tag> unit_tags;

    for (action_index = 0; action_index < raw->size(); action_index++) {
        action = (*raw)[action_index];
        ability_id = action.ability_id;
        unit_tags = action.unit_tags;
        switch (action.target_type) {
            case sc2::ActionRaw::TargetNone: {
                for (tag_index = 0; tag_index < unit_tags.size(); tag_index++) {
                    action_interface->UnitCommand(unit_tags[tag_index], ability_id);
                }
                break;
            }
            case sc2::ActionRaw::TargetUnitTag: {
                for (tag_index = 0; tag_index < unit_tags.size(); tag_index++) {
                    action_interface->UnitCommand(unit_tags[tag_index], ability_id, action.target_tag);
                }
                break;
            }
            case sc2::ActionRaw::TargetPosition: {
                for (tag_index = 0; tag_index < unit_tags.size(); tag_index++) {
                    action_interface->UnitCommand(unit_tags[tag_index], ability_id, action.target_point);
                }
                break;
            }
        }
    }
    action_interface.SendActions();
}

void run_next_raw_actions(Replay *replay_observer, ActionInterface* action_interface) {
    if (replay_observer->RawActionsEmpty()) {
        return;
    }
    RawActions raw = replay_observer->GetNextRawActions();
    run_raw_actions(&raw, action_interface);
}
