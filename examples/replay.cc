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
  int times_picked = 0;

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
    int num_players = ReplayControl()->GetReplayInfo().num_players;

    printf("PlayerID: %d ", obs->GetPlayerID());
    for (int i = 0; i < raw.size(); i++) {
      printf("Ability_id %d. ", raw[i].ability_id);
      for (int j = 0; j < raw[i].unit_tags.size(); j++) {
        printf("Tags %d ", raw[i].unit_tags[j]);
      }
    }
    raw_actions_array.push(raw); // make sure don't need reserve()
    if (raw.size() != 0)
      printf("LOOK AT ACTIONS %d \n", raw.size());
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
    if (++times_picked == 2) {
        raw_actions_array.pop();
        times_picked = 0;
    }
    return ret;
  }

  bool RawActionsEmpty() {
    return raw_actions_array.empty();
  }
  int RawActionsSize() {
    return raw_actions_array.size();
  }
};

void run_raw_actions(RawActions* raw, ActionInterface* action_interface) {
  int action_index = 0;
  int tag_index = 0;
  ActionRaw action;
  AbilityID ability_id;
  std::vector<Tag> unit_tags;
  if (raw->size() == 0) {
        return;
  }
  for (action_index = 0; action_index < raw->size(); action_index++) {
    action = (*raw)[action_index];
    ability_id = action.ability_id;
    unit_tags = action.unit_tags;
    printf("Ability_id %d", ability_id);

    switch (action.target_type) {
      case sc2::ActionRaw::TargetNone: {
        for (tag_index = 0; tag_index < unit_tags.size(); tag_index++) {
          printf("Tags %d", unit_tags[tag_index]);
          action_interface->UnitCommand(unit_tags[tag_index], ability_id);
        }
        break;
      }
      case sc2::ActionRaw::TargetUnitTag: {
        for (tag_index = 0; tag_index < unit_tags.size(); tag_index++) {
          printf("Tags %d", unit_tags[tag_index]);
          action_interface->UnitCommand(unit_tags[tag_index], ability_id, action.target_tag);
        }
        break;
      }
      case sc2::ActionRaw::TargetPosition: {
        for (tag_index = 0; tag_index < unit_tags.size(); tag_index++) {
          printf("Tags %d", unit_tags[tag_index]);
          action_interface->UnitCommand(unit_tags[tag_index], ability_id, action.target_point);
        }
        break;
      }
    }
  }
  printf("PUSHING LOOK AT ACTIONS %d \n", raw->size());
  action_interface->SendActions();
}

void run_next_raw_actions(Replay *replay_observer, ActionInterface* action_interface) {
  if (replay_observer->RawActionsEmpty()) {
    return;
  }
  RawActions raw = replay_observer->GetNextRawActions();
  run_raw_actions(&raw, action_interface);
}

class Bot : public Agent {
public:
  int stop_iter;
  Replay* replay_observer;
  Bot(): Agent() {}
  Bot(int i, Replay* o): Agent(), stop_iter(i), replay_observer(o)  {}

  virtual void OnGameStart() final {
    printf("Multiplayer game started \n");
  }

  virtual void OnStep() final {
    const sc2::ObservationInterface* obs = Observation();
    printf("PlayerID: %d ", obs->GetPlayerID());
    
    ActionInterface* action_interface = Actions();
    run_next_raw_actions(replay_observer, action_interface);
    printf("Multiplayer game stepping \n");
  }
};

void run_replay_coordinator(Replay *replay_observer, int stop_iter, int argc, char* argv[]) {
  sc2::Coordinator coordinator;
  int i = 0;
  printf("Starting replay \n ");
  if (!coordinator.LoadSettings(argc, argv)) {
    exit(1);
  }
  if (!coordinator.SetReplayPath(kReplayFolder)) {
    printf("Replay Folder: %s \n", kReplayFolder);
    std::cout << "Unable to find replays." << std::endl;
    exit(1);
  }
  coordinator.AddReplayObserver(replay_observer);
  
  while (++i < stop_iter){
    coordinator.Update();
  }
  coordinator.LeaveGame(); // doesn't do anything since no agents
}


int main(int argc, char* argv[]) {
  int stop_iter = 10;
  int i = 0;
  sc2::Coordinator replay_coordinator;
  sc2::Coordinator multiagent_coordinator;
  Replay replay_observer;
  Bot bot(i, &replay_observer);
  Bot bot1(i, &replay_observer);
  std::string map_name;

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
  
  replay_coordinator.Update();

  // can only call after first Update()
  map_name = std::string(replay_observer->ReplayControl()->GetReplayInfo().map_name.c_str());
  map_name.erase(remove_if(map_name.begin(), map_name.end(), isspace), map_name.end());
  printf("Map name: %s \n", map_name);


  while (++i < stop_iter){
    replay_coordinator.Update();
  }
  replay_coordinator.LeaveGame(); // doesn't do anything since no agents
  printf("Finished replay with %d ticks \n ", i);

  printf("Finished replay with %d ticks \n ", i);
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
  multiagent_coordinator.StartGame("Ladder/AbyssalReefLE.SC2Map");
  // multiagent_coordinator.StartGame(sc2::kMapAbyssalReefLE);
  //  multiagent_coordinator.StartGame(map_name);
  i = 0;
  while (i++ < stop_iter) {
    //printf("Iter: %d \n", i);
    multiagent_coordinator.Update();
  }
  printf("Finished multiplayer game \n ");
}

