#include <sc2api/sc2_api.h>

#include <iostream>

using namespace sc2;

class Bot : public Agent {
public:
    virtual void OnGameStart() final {
        std::cout << "BOT: In game start!\n" << std::endl;

    }
    virtual void OnStep() final {
        //std::cout << "BOT: Game step!\n" << std::endl;
        //std::cout << Observation()->GetGameLoop() << std::endl;
        const ObservationInterface* ob = Observation();
        printf("Worker: %d, Minerals: %d \n", ob->GetFoodWorkers(), ob->GetMinerals()); 
        TryBuildSupplyDepot();
        TryBuildBarracks();
    }
    virtual void OnUnitIdle(const Unit& unit) final {
        std::cout << "BOT: Unit is idle! Bot of type: " << std::endl;
        sc2::UNIT_TYPEID unit_type = unit.unit_type.ToType();
        printf("%d\n", unit_type);
        switch(unit_type) {
            case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
                Actions()-> UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
                break;
            }   
            case UNIT_TYPEID::TERRAN_SCV: {
                uint64_t mineral_target;
                if (!FindNearestMineralPatch(unit.pos, mineral_target)) {
                    break;
                }
                Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
                // SMART means click a unit and right click 
                break;
            }
            case UNIT_TYPEID::TERRAN_BARRACKS: {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
                break;      
            }
            case UNIT_TYPEID::TERRAN_MARINE: {
                const GameInfo& game_info = Observation()->GetGameInfo();
                Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, game_info.enemy_start_locations.front());
                break;
            }
            default: {
                break;
            }
        }
    }

    bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV) {
        const ObservationInterface* observation = Observation();

        // If a unit already is building a supply structure of this type, do nothing.
        // Also get an scv to build the structure.
        Unit unit_to_build;
        Units units = observation->GetUnits(Unit::Alliance::Self);
        for (const auto& unit : units) {
            for (const auto& order : unit.orders) {
                if (order.ability_id == ability_type_for_structure) {
                    // if being built, don't do anything
                    return false;
                }
            }
            if (unit.unit_type == unit_type) {
                unit_to_build = unit;
            }
        }

        float rx = GetRandomScalar();
        float ry = GetRandomScalar();

        Actions()->UnitCommand(unit_to_build,
            ability_type_for_structure,
            Point2D(unit_to_build.pos.x + rx * 15.0f, unit_to_build.pos.y + ry * 15.0f));

        return true;
    }
    
    // get units of a certain type
    size_t CountUnitType(UNIT_TYPEID unit_type) {
        return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }

    bool TryBuildBarracks() {
        const ObservationInterface* observation = Observation();

        if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1) {
            return false;
        }

        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > 0) {
            return false;
        }
        return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS);
    }

    bool TryBuildSupplyDepot() {
        const ObservationInterface* observation = Observation();

        // If we are not supply capped, don't build a supply depot.
        if (observation->GetFoodUsed() <= observation->GetFoodCap() - 2)
            return false;

        // Try and build a depot. Find a random SCV and give it the order.
        return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
    }

    bool FindNearestMineralPatch(const Point2D& start, uint64_t& target) {
        Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
        float distance = std::numeric_limits<float>::max();
        for (const auto& u: units) {
            if (u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
                float d = DistanceSquared2D(u.pos, start);
                if (d < distance) {
                    distance = d;
                    target = u.tag;
                }
            }
        }
        
        if (distance == std::numeric_limits<float>::max()) {
            return false;
        }

        return true;
    }
};

int main(int argc, char* argv[]) {
    printf("HELLO WORLD OH MY GOD\n");

    Coordinator coord;
    coord.LoadSettings(argc, argv);
    
    Bot bot;
    coord.SetParticipants({
        CreateParticipant(Race::Terran, &bot),
        CreateComputer(Race::Zerg)
    });

    coord.LaunchStarcraft();
    coord.StartGame(sc2::kMapBelShirVestigeLE);

    while (coord.Update()) {
        //printf("COORDINATOR: Updating! \n");
    }
    return 0;
}
