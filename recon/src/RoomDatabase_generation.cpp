#include "recon_all.h"
#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

bool RoomDatabase::roomsFitHorizontally(Room *left, Room *right) {
    // Expanded in randomizeRooms @0075ef0a-0075ef7f.
    auto **a = left->getTiles();
    auto **b = right->getTiles();
    for (int y = 0; y < left->getHeight(); ++y)
        if (a[left->getWidth() - 1][y].type != 1 && b[0][y].type != 1) return true;
    return false;
}

bool RoomDatabase::roomsFitVertically(Room *above, Room *below) {
    const int width = above->getWidth(), height = above->getHeight();
    auto **a = above->getTiles();
    auto **b = below->getTiles();
    for (int x = 0; x < width; ++x) {
        const int upper = a[x][height - 1].type, lower = b[x][0].type;
        if (upper == 1 || lower == 1) continue;
        bool approach = upper == 3 || upper == 6;
        const auto upperNeighbor = [&](int adjacent) {
            return (a[adjacent][height - 2].type == 1 && a[adjacent][height - 3].type != 1 &&
                    a[x][height - 2].type != 1 && a[x][height - 3].type != 1) ||
                   ((a[adjacent][height - 1].type == 1 || b[adjacent][0].type == 1) &&
                    a[x][height - 2].type != 1 && a[adjacent][height - 2].type != 1);
        };
        if (!approach && x < width - 1) approach = upperNeighbor(x + 1);
        if (!approach && x > 0) approach = upperNeighbor(x - 1);
        if (!approach) continue;
        const auto lowerNeighbor = [&](int adjacent) {
            return b[adjacent][0].type != 1 &&
                   (b[adjacent][1].type == 1 || b[adjacent][1].type == 6);
        };
        if (b[x][1].type == 1 || (x < width - 1 && lowerNeighbor(x + 1)) ||
            (x > 0 && lowerNeighbor(x - 1)) || lower == 3 || b[x][1].type == 6 || lower == 6)
            return true;
    }
    return false;
}

void RoomDatabase::randomizeRooms(Room ***output, Room ***models, int width, int height,
                                   bool requireFit) {
    std::map<std::string, int> used;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (output[x][y]) continue;
            auto *model = models[x][y];
            model->roomType = model->getRoomTypeBasedOnFlags();
            for (int attempt = 0;; ++attempt) {
                auto *selected = getRandomRoom(model, used);
                output[x][y] = new Room(selected ? selected : model);
                output[x][y]->x = x;
                output[x][y]->y = y;
                if (!selected) break;
                ++used[selected->uid];
                bool fits = true;
                if (requireFit) {
                    if (x > 0 && model->links[Room::LINK_LEFT])
                        fits = roomsFitHorizontally(output[x - 1][y], output[x][y]);
                    if (y > 0 && model->links[Room::LINK_UP])
                        fits = fits && roomsFitVertically(output[x][y - 1], output[x][y]);
                }
                if (fits) break;
                if (attempt >= 20) {
                    VLOG(LOG_ERROR) << "Tried more than 20 times to fit a room at " << x << ", "
                                   << y << ". Using non-fitting room now.";
                    break;
                }
                VLOG(LOG_DEBUG) << "Rerolling a room that doesn't fit(try " << attempt << ") at "
                               << x << ", " << y << ". Room Type: " << model->roomType;
                delete output[x][y];
                output[x][y] = nullptr;
            }
            // One allocated Room is shared by all cells of a multi-room template.
            auto *room = output[x][y];
            for (int dx = 0; dx < room->getWidthInRooms(); ++dx)
                for (int dy = 0; dy < room->getHeightInRooms(); ++dy)
                    if (dx || dy) output[x + dx][y + dy] = room;
        }
    }
}
