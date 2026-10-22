#include "recon_all.h"

#define VLOG(level) if ((level) > Log::reportingLevel()) ; else Log().get(level)

// Ghidra 00750f70: add a one-pixel duplicate border around each atlas tile.
int ResourceManager::processImage(const std::string &name, ProcessImageJob * const job,
                                  bool incremental) {
    const std::string processedName = "@" + name;
    sf::RenderTexture *output = job->postTextureCached;
    if (!output) {
        const auto found = processedTextures.find(processedName);
        if (found != processedTextures.end()) output = found->second;
    }

    const sf::Texture *source = job->preTextureCached;
    // The bool argument is passed in CL and lost in the decompiler output:
    // 0075103d: movb $1, %cl; 0075103f: call ResourceManager::getTexture.
    if (!source) source = getTexture(name, true);
    if (!source) {
        VLOG(LOG_ERROR) << "ResourceManager::processImage: failed to get preTexture for \""
                        << name << "\"";
        return -1;
    }

    const auto sourceSize = source->getSize();
    const int columns = static_cast<int>(sourceSize.x) / job->tileWidth;
    const int rows = static_cast<int>(sourceSize.y) / job->tileHeight;
    const int width = (job->tileWidth + 2) * columns;
    const int height = (job->tileHeight + 2) * rows;
    sf::Sprite sprite(*source);

    if (!output) {
        VLOG(LOG_INFO) << "ResourceManager::processImage: creating new RenderTexture for \""
                       << processedName << "\"";
        output = new sf::RenderTexture;
        processedTextures.insert({processedName, output});
        if (!output->create(width, height, false)) {
            VLOG(LOG_ERROR) << "ResourceManager::processImage: Failed to create RenderTexture("
                            << width << ", " << height << ")";
            return -2;
        }
        output->clear(sf::Color::Transparent);
    }

    job->postTextureCached = output;
    const int firstProcessed = job->tilesProcessed;
    const int firstRow = firstProcessed / columns;
    const int firstColumn = firstProcessed % columns;

    auto draw = [&](int x, int y, int w, int h, float destX, float destY) {
        sprite.setTextureRect(sf::IntRect(x, y, w, h));
        sprite.setPosition(destX, destY);
        output->draw(sprite, sf::RenderStates::Default);
    };

    for (int row = firstRow; row < rows; ++row) {
        // The original retains the initial column across rows (00751322,
        // 00751360); do not reset it when resuming an incremental job.
        for (int column = firstColumn; column < columns; ++column) {
            const int x = column * job->tileWidth;
            const int y = row * job->tileHeight;
            const int right = x + job->tileWidth - 1;
            const int bottom = y + job->tileHeight - 1;
            const float destX = static_cast<float>(column * (job->tileWidth + 2));
            const float destY = static_cast<float>(row * (job->tileHeight + 2));
            const float destRight = static_cast<float>(job->tileWidth + 1) + destX;
            const float destBottom = static_cast<float>(job->tileHeight + 1) + destY;
            const float destInnerX = destX + 1.f;
            const float destInnerY = destY + 1.f;

            // Preserve the original draw order: corners, vertical edges,
            // horizontal edges, then the complete tile.
            draw(x, y, 1, 1, destX, destY);
            draw(right, y, 1, 1, destRight, destY);
            draw(right, bottom, 1, 1, destRight, destBottom);
            draw(x, bottom, 1, 1, destX, destBottom);
            draw(x, y, 1, job->tileHeight, destX, destInnerY);
            draw(right, y, 1, job->tileHeight, destRight, destInnerY);
            draw(x, y, job->tileWidth, 1, destInnerX, destY);
            draw(x, bottom, job->tileWidth, 1, destInnerX, destBottom);
            draw(x, y, job->tileWidth, job->tileHeight, destInnerX, destInnerY);

            ++job->tilesProcessed;
            if (incremental && job->tilesProcessed - firstProcessed >= 16) break;
        }
        if (incremental && job->tilesProcessed - firstProcessed >= 16) break;
    }

    output->display();
    return columns * rows - job->tilesProcessed;
}
