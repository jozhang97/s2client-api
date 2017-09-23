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
  std::queue<RawActions> raw_actions_array;
  Replay() :
    sc2::ReplayObserver() {
  }
  void OnStep() final {
    const sc2::ObservationInterface* obs = Observation();
    const RawActions& raw = obs->GetRawActions();
    int num_players = ReplayControl()->GetReplayInfo().num_players;
    for (int i = 0; i < raw.size(); i++) {
      printf("Ability_id %d. ", raw[i].ability_id);
      for (int j = 0; j < raw[i].unit_tags.size(); j++) {
        printf("Tags %d ", raw[i].unit_tags[j]);
      }
    }
    if (raw.size() != 0) {
      printf("Replay: PlayerID: %d \n", obs->GetPlayerID());
    }
    raw_actions_array.push(raw); // make sure don't need reserve()
  }
  RawActions GetNextRawActions() {
    RawActions ret = raw_actions_array.front();
    raw_actions_array.pop();
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

  virtual void OnStep() final {
    const sc2::ObservationInterface* obs = Observation();
    printf("Agent: PlayerID: %d \n", obs->GetPlayerID());
    ActionInterface* action_interface = Actions();
    run_next_raw_actions(replay_observer, action_interface);
  }
};

int main(int argc, char* argv[]) {
  int stop_iter = 1000;
  int i = 0;
  sc2::Coordinator replay_coordinator, replay_coordinator2;
  sc2::Coordinator multiagent_coordinator;
  Replay replay_observer, replay_observer2;
  Bot bot(i, &replay_observer);
  Bot bot2(i, &replay_observer2);
  std::string map_name;
  char* used_map_name = "Ladder/AbyssalReefLE.SC2Map";

// --------------------------------------------------------------------------------
  printf("Starting replay first time \n ");
  if (!replay_coordinator.LoadSettings(argc, argv)) {
    return 1;
  }
  if (!replay_coordinator.SetReplayPath(kReplayFolder)) {
    std::cout << "Unable to find replays." << std::endl;
    return 1;
  }
  replay_coordinator.AddReplayObserver(&replay_observer);

  while (++i < stop_iter){
    replay_coordinator.Update();
  }
  i = 0;
  replay_coordinator.LeaveGame(); // doesn't do anything since no agents
// --------------------------------------------------------------------------------
  printf("Starting replay second time \n ");
  if (!replay_coordinator2.LoadSettings(argc, argv)) {
    return 1;
  }
  if (!replay_coordinator2.SetReplayPath(kReplayFolder)) {
    std::cout << "Unable to find replays." << std::endl;
    return 1;
  }
  replay_coordinator2.AddReplayObserver(&replay_observer2);

  while (++i < stop_iter){
    replay_coordinator2.Update();
  }
  i = 0;
  replay_coordinator2.LeaveGame(); // doesn't do anything since no agents
// --------------------------------------------------------------------------------
  // can only call after first Update()
  map_name = std::string(replay_observer.ReplayControl()->GetReplayInfo().map_name.c_str());
  map_name.erase(remove_if(map_name.begin(), map_name.end(), isspace), map_name.end());
  printf("Map name: %s \n", map_name.c_str());

// --------------------------------------------------------------------------------

  replay_coordinator.WaitForAllResponses();
  replay_coordinator2.WaitForAllResponses();
  multiagent_coordinator.WaitForAllResponses();
  while (!sc2::PollKeyPress());

// --------------------------------------------------------------------------------

  printf("Starting multiplayer game \n ");
  if (!multiagent_coordinator.LoadSettings(argc, argv)) {
    return 1;
  }
  multiagent_coordinator.SetParticipants({
    CreateParticipant(Race::Terran, &bot),
    CreateParticipant(Race::Terran, &bot2)
  });
  multiagent_coordinator.LaunchStarcraft();
  multiagent_coordinator.StartGame(used_map_name);
  // multiagent_coordinator.StartGame(sc2::kMapAbyssalReefLE);
  //  multiagent_coordinator.StartGame(map_name);
  while (i++ < stop_iter) {
    multiagent_coordinator.Update();
  }
  i = 0;
  printf("Finished multiplayer game \n ");
}

