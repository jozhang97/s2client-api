#include <sc2api/sc2_api.h>

#include <iostream>

using namespace sc2;

class Bot : public Agent {
public:
    virtual void OnGameStart() final {
        std::cout << "OnGameStart: BOT: In game start!\n" << std::endl;

    }
    virtual void OnStep() final {
        const ObservationInterface* ob = Observation();
        printf("OnStep: Worker: %d, Minerals: %d \n", ob->GetFoodWorkers(), ob->GetMinerals()); 
    }

    virtual void OnUnitIdle(const Unit& unit) final {
        std::cout << "OnUnitIdle: BOT: Unit is idle! Bot of type: " << std::endl;
        sc2::UNIT_TYPEID unit_type = unit.unit_type.ToType();
    }
};




int main(int argc, char* argv[]) {
    printf("HELLO WORLD OH MY GOD\n");

    Coordinator coord;
    if (!coord.LoadSettings(argc, argv)) { 
        return false; 
    }
    
    Bot bot;
    Bot bot_extra;
    coord.SetParticipants({
        CreateParticipant(Race::Terran, &bot),
        CreateParticipant(Race::Terran, &bot_extra)
    });

    coord.LaunchStarcraft();
    coord.StartGame(sc2::kMapBelShirVestigeLE);

    int i = 0;
    while (i++ < 1000) {
        coord.Update();
        printf("COORDINATOR: Updating! \n");
    }
    return 0;
}
