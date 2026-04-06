#include "Chunk.h"

#include <GL/gl.h>
#include <string.h>

#include <cmath>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "LevelRenderer.h"
#include "TileRenderer.h"
#include "app/include/FrameProfiler.h"
#include "app/linux/LinuxGame.h"
#include "app/linux/Stubs/winapi_stubs.h"
#include "minecraft/SharedConstants.h"
#include "minecraft/client/renderer/GameRenderer.h"
#include "minecraft/client/renderer/Tesselator.h"
#include "minecraft/client/renderer/culling/Culler.h"
#include "minecraft/client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "minecraft/world/Icon.h"
#include "minecraft/world/entity/Entity.h"
#include "minecraft/world/level/Level.h"
#include "minecraft/world/level/LevelSource.h"
#include "minecraft/world/level/Region.h"
#include "minecraft/world/level/chunk/LevelChunk.h"
#include "minecraft/world/level/tile/LiquidTile.h"
#include "minecraft/world/level/tile/Tile.h"
#include "minecraft/world/level/tile/entity/TileEntity.h"
#include "minecraft/world/phys/AABB.h"
#include "platform/sdl2/Render.h"

int Chunk::updates = 0;

#if defined(_LARGE_WORLDS)
thread_local uint8_t* Chunk::m_tlsTileIds = nullptr;

void Chunk::CreateNewThreadStorage() {
    m_tlsTileIds = new unsigned char[16 * 16 * Level::maxBuildHeight];
}

void Chunk::ReleaseThreadStorage() { delete m_tlsTileIds; }

uint8_t* Chunk::GetTileIdsStorage() { return m_tlsTileIds; }
#else
// 4J Stu - Don't want this when multi-threaded
Tesselator* Chunk::t = Tesselator::getInstance();
#endif
LevelRenderer* Chunk::levelRenderer;

void Chunk::reconcileRenderableTileEntities(
    const std::vector<std::shared_ptr<TileEntity> >& renderableTileEntities) {
    int key =
        levelRenderer->getGlobalIndexForChunk(this->x, this->y, this->z, level);
    auto it = globalRenderableTileEntities->find(key);
    if (!renderableTileEntities.empty()) {
        std::unordered_set<TileEntity*> currentRenderableTileEntitySet;
        currentRenderableTileEntitySet.reserve(renderableTileEntities.size());
        for (size_t i = 0; i < renderableTileEntities.size(); i++) {
            currentRenderableTileEntitySet.insert(
                renderableTileEntities[i].get());
        }

        if (it != globalRenderableTileEntities->end()) {
            LevelRenderer::RenderableTileEntityBucket& existingBucket =
                it->second;

            for (auto it2 = existingBucket.tiles.begin();
                 it2 != existingBucket.tiles.end(); it2++) {
                TileEntity* tileEntity = (*it2).get();
                if (currentRenderableTileEntitySet.find(tileEntity) ==
                    currentRenderableTileEntitySet.end()) {
                    (*it2)->setRenderRemoveStage(
                        TileEntity::e_RenderRemoveStageFlaggedAtChunk);
                    levelRenderer->queueRenderableTileEntityForRemoval_Locked(
                        key, tileEntity);
                } else {
                    (*it2)->setRenderRemoveStage(
                        TileEntity::e_RenderRemoveStageKeep);
                }
            }

            for (size_t i = 0; i < renderableTileEntities.size(); i++) {
                renderableTileEntities[i]->setRenderRemoveStage(
                    TileEntity::e_RenderRemoveStageKeep);
                if (existingBucket.indexByTile.find(
                        renderableTileEntities[i].get()) ==
                    existingBucket.indexByTile.end()) {
                    levelRenderer->addRenderableTileEntity_Locked(
                        key, renderableTileEntities[i]);
                }
            }
        } else {
            for (size_t i = 0; i < renderableTileEntities.size(); i++) {
                renderableTileEntities[i]->setRenderRemoveStage(
                    TileEntity::e_RenderRemoveStageKeep);
                levelRenderer->addRenderableTileEntity_Locked(
                    key, renderableTileEntities[i]);
            }
        }
    } else if (it != globalRenderableTileEntities->end()) {
        for (auto it2 = it->second.tiles.begin(); it2 != it->second.tiles.end();
             it2++) {
            (*it2)->setRenderRemoveStage(
                TileEntity::e_RenderRemoveStageFlaggedAtChunk);
            levelRenderer->queueRenderableTileEntityForRemoval_Locked(
                key, (*it2).get());
        }
    }
}

// TODO - 4J see how input entity vector is set up and decide what way is best
// to pass this to the function
Chunk::Chunk(Level* level, LevelRenderer::rteMap& globalRenderableTileEntities,
             std::mutex& globalRenderableTileEntities_cs, int x, int y, int z,
             ClipChunk* clipChunk)
    : globalRenderableTileEntities(&globalRenderableTileEntities),
      globalRenderableTileEntities_cs(&globalRenderableTileEntities_cs) {
    clipChunk->visible = false;
    const double g = 6;
    bb = AABB(-g, -g, -g, XZSIZE + g, SIZE + g, XZSIZE + g);
    id = 0;

    this->level = level;
    // this->globalRenderableTileEntities = globalRenderableTileEntities;

    assigned = false;
    this->clipChunk = clipChunk;
    setPos(x, y, z);
}

void Chunk::setPos(int x, int y, int z) {
    if (assigned && (x == this->x && y == this->y && z == this->z)) return;

    reset();

    this->x = x;
    this->y = y;
    this->z = z;
    xm = x + XZSIZE / 2;
    ym = y + SIZE / 2;
    zm = z + XZSIZE / 2;
    clipChunk->xm = xm;
    clipChunk->ym = ym;
    clipChunk->zm = zm;

    clipChunk->globalIdx =
        LevelRenderer::getGlobalIndexForChunk(x, y, z, level);
    levelRenderer->setGlobalChunkConnectivity(clipChunk->globalIdx, ~0ULL);

    // 4J - we're not using offsetted renderlists anymore, so just set the full
    // position of this chunk into x/y/zRenderOffs where it will be used
    // directly in the renderlist of this chunk
    xRenderOffs = x;
    yRenderOffs = y;
    zRenderOffs = z;
    xRender = 0;
    yRender = 0;
    zRender = 0;

    float g = 6.0f;

    clipChunk->aabb[0] = bb.x0 + x;
    clipChunk->aabb[1] = bb.y0 + y;
    clipChunk->aabb[2] = bb.z0 + z;
    clipChunk->aabb[3] = bb.x1 + x;
    clipChunk->aabb[4] = bb.y1 + y;
    clipChunk->aabb[5] = bb.z1 + z;

    assigned = true;

    {
        std::lock_guard<std::recursive_mutex> lock(
            levelRenderer->m_csDirtyChunks);
        unsigned char refCount =
            levelRenderer->incGlobalChunkRefCount(x, y, z, level);
        //	printf("\t\t [inc] refcount %d at %d, %d, %d\n",refCount,x,y,z);

        //	int idx = levelRenderer->getGlobalIndexForChunk(x, y, z, level);

        // If we're the first thing to be referencing this, mark it up as dirty
        // to get rebuilt
        if (refCount == 1) {
            //		printf("Setting %d %d %d dirty [%d]\n",x,y,z, idx);
            // Chunks being made dirty in this way can be very numerous (eg the
            // full visible area of the world at start up, or a whole edge of
            // the world when moving). On account of this, don't want to stick
            // them into our lock free queue that we would normally use for
            // letting the render update thread know about this chunk. Instead,
            // just set the flag to say this is dirty, and then pass a special
            // value of 1 through to the lock free stack which lets that thread
            // know that at least one chunk other than the ones in the stack
            // itself have been made dirty.
            levelRenderer->setGlobalChunkFlag(x, y, z, level,
                                              LevelRenderer::CHUNK_FLAG_DIRTY);
        }
    }
}

void Chunk::translateToPos() {
    glTranslatef((float)xRenderOffs, (float)yRenderOffs, (float)zRenderOffs);
}

Chunk::Chunk() {}

void Chunk::makeCopyForRebuild(Chunk* source) {
    this->level = source->level;
    this->x = source->x;
    this->y = source->y;
    this->z = source->z;
    this->xRender = source->xRender;
    this->yRender = source->yRender;
    this->zRender = source->zRender;
    this->xRenderOffs = source->xRenderOffs;
    this->yRenderOffs = source->yRenderOffs;
    this->zRenderOffs = source->zRenderOffs;
    this->xm = source->xm;
    this->ym = source->ym;
    this->zm = source->zm;
    this->bb = source->bb;
    this->clipChunk = nullptr;
    this->id = source->id;
    this->globalRenderableTileEntities = source->globalRenderableTileEntities;
    this->globalRenderableTileEntities_cs =
        source->globalRenderableTileEntities_cs;
}

void Chunk::rebuild() {
    //	if (!dirty) return;

#if defined(_LARGE_WORLDS)
    Tesselator* t = Tesselator::getInstance();
#else
    Chunk::t = Tesselator::getInstance();  // 4J - added - static initialiser
                                           // being set at the wrong time
#endif

    updates++;

    int x0 = x;
    int y0 = y;
    int z0 = z;
    int x1 = x + XZSIZE;
    int y1 = y + SIZE;
    int z1 = z + XZSIZE;

    LevelChunk::touchedSky = false;

    //	unordered_set<shared_ptr<TileEntity> >
    // oldTileEntities(renderableTileEntities.begin(),renderableTileEntities.end());
    //// 4J removed this & next line 	renderableTileEntities.clear();

    std::vector<std::shared_ptr<TileEntity> >
        renderableTileEntities;  // 4J - added

    int r = 1;

    int lists = levelRenderer->getGlobalIndexForChunk(this->x, this->y, this->z,
                                                      level) *
                2;
    lists += levelRenderer->chunkLists;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 4J - optimisation begins.

    // Get the data for the level chunk that this render chunk is it (level
    // chunk is 16 x 16 x 128, render chunk is 16 x 16 x 16. We wouldn't have to
    // actually get all of it if the data was ordered differently, but currently
    // it is ordered by x then z then y so just getting a small range of y out
    // of it would involve getting the whole thing into the cache anyway.

#if defined(_LARGE_WORLDS)
    unsigned char* tileIds = GetTileIdsStorage();
#else
    static unsigned char tileIds[16 * 16 * Level::maxBuildHeight];
#endif
    std::vector<uint8_t> tileArray(16 * 16 * Level::maxBuildHeight);
    level->getChunkAt(x, z)->getBlockData(tileArray);
    memcpy(
        tileIds, tileArray.data(),
        16 * 16 * Level::maxBuildHeight);  // 4J - TODO - now our data has been
                                           // re-arranged, we could just extra
                                           // the vertical slice of this chunk
                                           // rather than the whole thing

    LevelSource* region =
        new Region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r, r);
    TileRenderer* tileRenderer =
        new TileRenderer(region, this->x, this->y, this->z, tileIds);

    // AP - added a caching system for Chunk::rebuild to take advantage of
    // Basically we're storing of copy of the tileIDs array inside the region so
    // that calls to Region::getTile can grab data more quickly from this array
    // rather than calling CompressedTileStorage. On the Vita the total thread
    // time spent in Region::getTile went from 20% to 4%.

    // We now go through the vertical section of this level chunk that we are
    // interested in and try and establish (1) if it is completely empty (2) if
    // any of the tiles can be quickly determined to not need rendering because
    // they are in the middle of other tiles and
    //     so can't be seen. A large amount (> 60% in tests) of tiles that call
    //     tesselateInWorld in the unoptimised version of this function fall
    //     into this category. By far the largest category of these are tiles in
    //     solid regions of rock.
    bool empty = true;
    {
        FRAME_PROFILE_SCOPE(ChunkPrepass);
        for (int yy = y0; yy < y1; yy++) {
            for (int zz = 0; zz < 16; zz++) {
                for (int xx = 0; xx < 16; xx++) {
                    // 4J Stu - tile data is ordered in 128 blocks of full
                    // width, lower 128 then upper 128
                    int indexY = yy;
                    int offset = 0;
                    if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                        indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                        offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
                    }

                    unsigned char tileId =
                        tileIds[offset + (((xx + 0) << 11) | ((zz + 0) << 7) |
                                          (indexY + 0))];
                    if (tileId > 0) empty = false;

                    // Don't bother trying to work out neighbours for this tile
                    // if we are at the edge of the chunk - apart from the very
                    // bottom of the world where we shouldn't ever be able to
                    // see
                    if (yy == (Level::maxBuildHeight - 1)) continue;
                    if ((xx == 0) || (xx == 15)) continue;
                    if ((zz == 0) || (zz == 15)) continue;

                    // Establish whether this tile and its neighbours are all
                    // made of rock, dirt, unbreakable tiles, or have already
                    // been determined to meet this criteria themselves and have
                    // a tile of 255 set.
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    tileId = tileIds[offset + (((xx - 1) << 11) |
                                               ((zz + 0) << 7) | (indexY + 0))];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    tileId = tileIds[offset + (((xx + 1) << 11) |
                                               ((zz + 0) << 7) | (indexY + 0))];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    tileId = tileIds[offset + (((xx + 0) << 11) |
                                               ((zz - 1) << 7) | (indexY + 0))];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    tileId = tileIds[offset + (((xx + 0) << 11) |
                                               ((zz + 1) << 7) | (indexY + 0))];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;
                    // Treat the bottom of the world differently - we shouldn't
                    // ever be able to look up at this, so consider tiles as
                    // invisible if they are surrounded on sides other than the
                    // bottom
                    if (yy > 0) {
                        int indexYMinusOne = yy - 1;
                        int yMinusOneOffset = 0;
                        if (indexYMinusOne >=
                            Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                            indexYMinusOne -=
                                Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                            yMinusOneOffset =
                                Level::COMPRESSED_CHUNK_SECTION_TILES;
                        }
                        tileId = tileIds[yMinusOneOffset + (((xx + 0) << 11) |
                                                            ((zz + 0) << 7) |
                                                            indexYMinusOne)];
                        if (!((tileId == Tile::stone_Id) ||
                              (tileId == Tile::dirt_Id) ||
                              (tileId == Tile::unbreakable_Id) ||
                              (tileId == 255)))
                            continue;
                    }
                    int indexYPlusOne = yy + 1;
                    int yPlusOneOffset = 0;
                    if (indexYPlusOne >=
                        Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                        indexYPlusOne -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                        yPlusOneOffset = Level::COMPRESSED_CHUNK_SECTION_TILES;
                    }
                    tileId = tileIds[yPlusOneOffset + (((xx + 0) << 11) |
                                                       ((zz + 0) << 7) |
                                                       indexYPlusOne)];
                    if (!((tileId == Tile::stone_Id) ||
                          (tileId == Tile::dirt_Id) ||
                          (tileId == Tile::unbreakable_Id) || (tileId == 255)))
                        continue;

                    // This tile is surrounded. Flag it as not requiring to be
                    // rendered by setting its id to 255.
                    tileIds[offset + (((xx + 0) << 11) | ((zz + 0) << 7) |
                                      (indexY + 0))] = 0xff;
                }
            }
        }
    }

    // Nothing at all to do for this chunk?
    if (empty) {
        // 4J - added - clear any renderer data associated with this
        for (int currentLayer = 0; currentLayer < 2; currentLayer++) {
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level,
                                              LevelRenderer::CHUNK_FLAG_EMPTY0,
                                              currentLayer);
            RenderManager.CBuffClear(lists + currentLayer);
        }

        int globalIdx = levelRenderer->getGlobalIndexForChunk(this->x, this->y,
                                                              this->z, level);
        levelRenderer->setGlobalChunkConnectivity(globalIdx, ~0ULL);
        levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level,
                                          LevelRenderer::CHUNK_FLAG_COMPILED);

        delete region;
        delete tileRenderer;
        return;
    }
    // 4J - optimisation ends
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Binary Mesh Greedy implementation
    // https://gedge.ca/blog/2014-08-17-greedy-voxel-meshing/
    // https://github.com/cgerikj/binary-greedy-meshing/tree/master
    // https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/
    // also https://github.com/whleucka/voxel/blob/main/src/chunk/chunk_mesh.cpp
    //                                                                 ^ real useful thanks
    // also known as the "spongebob! why doesn't my AO work?"
    int greedyEligibleCount = 0;
    std::vector<uint8_t> greedyEligible(16 * 16 * 16, 0);
    std::vector<uint8_t> greedyLiquidTop(16 * 16 * 16, 0);
#if defined(ENABLE_GREEDY_MESHING)
    {
        const double greedyEps = 1e-6;
        auto greedyIndex = [](int lx, int ly, int lz) {
            return (ly << 8) | (lz << 4) | lx;
        };

        for (int z = z0; z < z1; z++) {
            for (int x = x0; x < x1; x++) {
                for (int y = y0; y < y1; y++) {
                    int indexY = y;
                    int offset = 0;
                    if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                        indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                        offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
                    }

                    unsigned char tileId =
                        tileIds[offset +
                                (((x - x0) << 11) | ((z - z0) << 7) | indexY)];
                    if (tileId == 0 || tileId == 0xff) continue;

                    Tile* tile = Tile::tiles[tileId];
                    if (tile == nullptr) continue;
                    if (tile->getRenderLayer() != 0) continue;
                    if (tile->getRenderShape() != Tile::SHAPE_BLOCK) continue;
                    if (tile->isEntityTile()) continue;
                    if (!tile->isSolidRender()) continue;
                    if (Tile::transculent[tileId]) continue;
                    if (tileId == Tile::grass_Id) continue;

                    tile->updateShape(level, x, y, z, -1,
                                      std::shared_ptr<TileEntity>());

                    double x0s = tile->getShapeX0();
                    double x1s = tile->getShapeX1();
                    double y0s = tile->getShapeY0();
                    double y1s = tile->getShapeY1();
                    double z0s = tile->getShapeZ0();
                    double z1s = tile->getShapeZ1();

                    if (std::fabs(x0s) > greedyEps ||
                        std::fabs(y0s) > greedyEps ||
                        std::fabs(z0s) > greedyEps ||
                        std::fabs(x1s - 1.0) > greedyEps ||
                        std::fabs(y1s - 1.0) > greedyEps ||
                        std::fabs(z1s - 1.0) > greedyEps)
                        continue;

                    bool ok = true;
                    for (int face = 0; face < 6; face++) {
                        if (tile->getTexture(level, x, y, z, face) == nullptr) {
                            ok = false;
                            break;
                        }
                    }
                    if (!ok) continue;

                    int lx = x - x0;
                    int ly = y - y0;
                    int lz = z - z0;
                    greedyEligible[greedyIndex(lx, ly, lz)] = 1;
                    greedyEligibleCount++;
                }
            }
        }
    }
    tileRenderer->setWaterTopSkipMask(greedyLiquidTop.data());
#else
    tileRenderer->setWaterTopSkipMask(nullptr);
#endif

    Tesselator::Bounds bounds;  // 4J MGH - added
    {
        // this was the old default clip bounds for the chunk, set in
        // Chunk::setPos.
        float g = 6.0f;
        bounds.boundingBox[0] = -g;
        bounds.boundingBox[1] = -g;
        bounds.boundingBox[2] = -g;
        bounds.boundingBox[3] = XZSIZE + g;
        bounds.boundingBox[4] = SIZE + g;
        bounds.boundingBox[5] = XZSIZE + g;
    }
    for (int currentLayer = 0; currentLayer < 2; currentLayer++) {
        bool renderNextLayer = false;
        bool rendered = false;
        // second part of the greedy mesh implementation
        bool listStarted = false;
        bool normalTessActive = false;
        bool greedyTessActive = false;
        int greedyMergedQuads = 0;
        int greedyTilesEmitted = 0;
        int greedyLiquidMergedQuads = 0;
        int greedyLiquidTilesEmitted = 0;

        auto startListIfNeeded = [&]() {
            if (listStarted) return;
            listStarted = true;

            glNewList(lists + currentLayer, GL_COMPILE);
            glDepthMask(true);
        };

        auto beginNormal = [&]() {
            if (normalTessActive) return;
            startListIfNeeded();

            t->useCompactVertices(true);
            t->useTileUV(false);
            t->begin();
            t->offset((float)(-this->x), (float)(-this->y), (float)(-this->z));
            normalTessActive = true;
        };

        auto endNormal = [&]() {
            if (!normalTessActive) return;
            t->end();
            bounds.addBounds(t->bounds);
            t->offset(0, 0, 0);
            normalTessActive = false;
        };

        auto beginGreedy = [&]() {
            if (greedyTessActive) return;
            startListIfNeeded();

            t->useCompactVertices(false);
            t->useTileUV(true);
            t->begin();
            t->offset((float)(-this->x), (float)(-this->y), (float)(-this->z));
            greedyTessActive = true;
        };

        auto endGreedy = [&]() {
            if (!greedyTessActive) return;
            t->end();
            bounds.addBounds(t->bounds);
            t->useTileUV(false);
            t->offset(0, 0, 0);
            greedyTessActive = false;
        };

        auto greedyIndex = [](int lx, int ly, int lz) {
            return (ly << 8) | (lz << 4) | lx;
        };

        auto getTileIdLocal = [&](int lx, int ly, int lz) -> unsigned char {
            int worldY = y0 + ly;
            int indexY = worldY;
            int offset = 0;
            if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
            }
            return tileIds[offset +
                           (((lx + 0) << 11) | ((lz + 0) << 7) | indexY)];
        };
        // bread taste better than key
        auto computeFaceKey = [&](Tile* tile, int wx, int wy, int wz, int face,
                                  uint32_t& colorKey, int& lightKey) {
            int col = tile->getColor(level, wx, wy, wz);
            float r = ((col >> 16) & 0xff) / 255.0f;
            float g = ((col >> 8) & 0xff) / 255.0f;
            float b = ((col) & 0xff) / 255.0f;


            if (GameRenderer::anaglyph3d) {
                // todo: this will DEF break anaglyph rendering
                // do we still need it?????????
            }

            float shade = 1.0f;
            switch (face) {
                case 0:
                    shade = 0.5f;
                    break;
                case 1:
                    shade = 1.0f;
                    break;
                case 2:
                case 3:
                    shade = 0.8f;
                    break;
                case 4:
                case 5:
                    shade = 0.6f;
                    break;
                default:
                    break;
            }

            r *= shade;
            g *= shade;
            b *= shade;

            if (SharedConstants::TEXTURE_LIGHTING) {
                int nx = wx, ny = wy, nz = wz;
                if (face == 0) ny--;
                if (face == 1) ny++;
                if (face == 2) nz--;
                if (face == 3) nz++;
                if (face == 4) nx--;
                if (face == 5) nx++;
                lightKey = tileRenderer->getLightColor(tile, level, nx, ny, nz);
            } else {
                int nx = wx, ny = wy, nz = wz;
                if (face == 0) ny--;
                if (face == 1) ny++;
                if (face == 2) nz--;
                if (face == 3) nz++;
                if (face == 4) nx--;
                if (face == 5) nx++;
                float br = tile->getBrightness(level, nx, ny, nz);
                r *= br;
                g *= br;
                b *= br;
                lightKey = 0;
            }

            auto clampByte = [](float v) -> uint8_t {
                int iv = (int)(v * 255.0f + 0.5f);
                if (iv < 0) iv = 0;
                if (iv > 255) iv = 255;
                return (uint8_t)iv;
            };

            colorKey =
                (clampByte(r) << 16) | (clampByte(g) << 8) | (clampByte(b));
        };

        auto shadeAt = [&](Tile* tile, int ax, int ay, int az) {
            return tileRenderer->getShadeBrightness(tile, level, ax, ay, az);
        };

        auto aoCorner = [&](float s1, float s2, float s3) {
            if (s1 < 1.0f && s2 < 1.0f) return 0.2f;
            return (s1 + s2 + s3) / 3.0f;
        };

        auto computeFaceAO = [&](Tile* tile, int wx, int wy, int wz, int face,
                                 float outAo[4]) {
            switch (face) {
                case 0: {  // Down (y-1)
                    float sW = shadeAt(tile, wx - 1, wy - 1, wz);
                    float sE = shadeAt(tile, wx + 1, wy - 1, wz);
                    float sN = shadeAt(tile, wx, wy - 1, wz - 1);
                    float sS = shadeAt(tile, wx, wy - 1, wz + 1);
                    float sWN = shadeAt(tile, wx - 1, wy - 1, wz - 1);
                    float sWS = shadeAt(tile, wx - 1, wy - 1, wz + 1);
                    float sEN = shadeAt(tile, wx + 1, wy - 1, wz - 1);
                    float sES = shadeAt(tile, wx + 1, wy - 1, wz + 1);

                    outAo[0] = aoCorner(sW, sS, sWS);
                    outAo[1] = aoCorner(sW, sN, sWN);
                    outAo[2] = aoCorner(sE, sN, sEN);
                    outAo[3] = aoCorner(sE, sS, sES);
                } break;
                case 1: {  // Up (y+1)
                    float sW = shadeAt(tile, wx - 1, wy + 1, wz);
                    float sE = shadeAt(tile, wx + 1, wy + 1, wz);
                    float sN = shadeAt(tile, wx, wy + 1, wz - 1);
                    float sS = shadeAt(tile, wx, wy + 1, wz + 1);
                    float sWN = shadeAt(tile, wx - 1, wy + 1, wz - 1);
                    float sWS = shadeAt(tile, wx - 1, wy + 1, wz + 1);
                    float sEN = shadeAt(tile, wx + 1, wy + 1, wz - 1);
                    float sES = shadeAt(tile, wx + 1, wy + 1, wz + 1);

                    outAo[0] = aoCorner(sE, sS, sES);
                    outAo[1] = aoCorner(sE, sN, sEN);
                    outAo[2] = aoCorner(sW, sN, sWN);
                    outAo[3] = aoCorner(sW, sS, sWS);
                } break;
                case 2: {  // North (z-1)
                    float sW = shadeAt(tile, wx - 1, wy, wz - 1);
                    float sE = shadeAt(tile, wx + 1, wy, wz - 1);
                    float sU = shadeAt(tile, wx, wy + 1, wz - 1);
                    float sD = shadeAt(tile, wx, wy - 1, wz - 1);
                    float sWU = shadeAt(tile, wx - 1, wy + 1, wz - 1);
                    float sWD = shadeAt(tile, wx - 1, wy - 1, wz - 1);
                    float sEU = shadeAt(tile, wx + 1, wy + 1, wz - 1);
                    float sED = shadeAt(tile, wx + 1, wy - 1, wz - 1);

                    outAo[0] = aoCorner(sW, sU, sWU);
                    outAo[1] = aoCorner(sE, sU, sEU);
                    outAo[2] = aoCorner(sE, sD, sED);
                    outAo[3] = aoCorner(sW, sD, sWD);
                } break;
                case 3: {  // South (z+1)
                    float sW = shadeAt(tile, wx - 1, wy, wz + 1);
                    float sE = shadeAt(tile, wx + 1, wy, wz + 1);
                    float sU = shadeAt(tile, wx, wy + 1, wz + 1);
                    float sD = shadeAt(tile, wx, wy - 1, wz + 1);
                    float sWU = shadeAt(tile, wx - 1, wy + 1, wz + 1);
                    float sWD = shadeAt(tile, wx - 1, wy - 1, wz + 1);
                    float sEU = shadeAt(tile, wx + 1, wy + 1, wz + 1);
                    float sED = shadeAt(tile, wx + 1, wy - 1, wz + 1);

                    outAo[0] = aoCorner(sW, sU, sWU);
                    outAo[1] = aoCorner(sW, sD, sWD);
                    outAo[2] = aoCorner(sE, sD, sED);
                    outAo[3] = aoCorner(sE, sU, sEU);
                } break;
                case 4: {  // West (x-1)
                    float sN = shadeAt(tile, wx - 1, wy, wz - 1);
                    float sS = shadeAt(tile, wx - 1, wy, wz + 1);
                    float sU = shadeAt(tile, wx - 1, wy + 1, wz);
                    float sD = shadeAt(tile, wx - 1, wy - 1, wz);
                    float sNU = shadeAt(tile, wx - 1, wy + 1, wz - 1);
                    float sND = shadeAt(tile, wx - 1, wy - 1, wz - 1);
                    float sSU = shadeAt(tile, wx - 1, wy + 1, wz + 1);
                    float sSD = shadeAt(tile, wx - 1, wy - 1, wz + 1);

                    outAo[0] = aoCorner(sS, sU, sSU);
                    outAo[1] = aoCorner(sN, sU, sNU);
                    outAo[2] = aoCorner(sN, sD, sND);
                    outAo[3] = aoCorner(sS, sD, sSD);
                } break;
                case 5: {  // East (x+1)
                    float sN = shadeAt(tile, wx + 1, wy, wz - 1);
                    float sS = shadeAt(tile, wx + 1, wy, wz + 1);
                    float sU = shadeAt(tile, wx + 1, wy + 1, wz);
                    float sD = shadeAt(tile, wx + 1, wy - 1, wz);
                    float sNU = shadeAt(tile, wx + 1, wy + 1, wz - 1);
                    float sND = shadeAt(tile, wx + 1, wy - 1, wz - 1);
                    float sSU = shadeAt(tile, wx + 1, wy + 1, wz + 1);
                    float sSD = shadeAt(tile, wx + 1, wy - 1, wz + 1);

                    outAo[0] = aoCorner(sS, sD, sSD);
                    outAo[1] = aoCorner(sN, sD, sND);
                    outAo[2] = aoCorner(sN, sU, sNU);
                    outAo[3] = aoCorner(sS, sU, sSU);
                } break;
                default:
                    outAo[0] = outAo[1] = outAo[2] = outAo[3] = 1.0f;
                    break;
            }
        };

        struct GreedyFaceKey {
            Tile* tile;
            Icon* tex;
            uint32_t colorKey;
            uint32_t aoKey;
            int lightKey;
            bool mipmap;
            bool operator==(const GreedyFaceKey& other) const {
                return tile == other.tile && tex == other.tex &&
                       colorKey == other.colorKey && aoKey == other.aoKey &&
                       lightKey == other.lightKey && mipmap == other.mipmap;
            }
        };

        auto greedyMeshFace = [&](int face) {
            const int U = 16;
            const int V = 16;
            std::vector<uint8_t> valid(U * V, 0);
            std::vector<GreedyFaceKey> keys(U * V);
            bool faceRendered = false;

            for (int slice = 0; slice < 16; slice++) {
                memset(valid.data(), 0, valid.size());

                for (int v = 0; v < V; v++) {
                    for (int u = 0; u < U; u++) {
                        int lx = 0, ly = 0, lz = 0;
                        switch (face) {
                            case 0:
                            case 1:
                                lx = u;
                                ly = slice;
                                lz = v;
                                break;
                            case 2:
                            case 3:
                                lx = u;
                                ly = v;
                                lz = slice;
                                break;
                            case 4:
                            case 5:
                                lx = slice;
                                ly = v;
                                lz = u;
                                break;
                        }

                        int idx = greedyIndex(lx, ly, lz);
                        if (greedyEligible[idx] == 0) continue;

                        int wx = x0 + lx;
                        int wy = y0 + ly;
                        int wz = z0 + lz;

                        unsigned char tileId = getTileIdLocal(lx, ly, lz);
                        if (tileId == 0 || tileId == 0xff) continue;

                        Tile* tile = Tile::tiles[tileId];
                        if (tile == nullptr) continue;
                        if (tile->getRenderLayer() != currentLayer) continue;

                        bool visible = false;
                        if (face == 0)
                            visible = tile->shouldRenderFace(level, wx, wy - 1,
                                                             wz, 0);
                        else if (face == 1)
                            visible = tile->shouldRenderFace(level, wx, wy + 1,
                                                             wz, 1);
                        else if (face == 2)
                            visible = tile->shouldRenderFace(level, wx, wy,
                                                             wz - 1, 2);
                        else if (face == 3)
                            visible = tile->shouldRenderFace(level, wx, wy,
                                                             wz + 1, 3);
                        else if (face == 4)
                            visible = tile->shouldRenderFace(level, wx - 1, wy,
                                                             wz, 4);
                        else if (face == 5)
                            visible = tile->shouldRenderFace(level, wx + 1, wy,
                                                             wz, 5);

                        if (!visible) continue;

                        Icon* tex = tile->getTexture(level, wx, wy, wz, face);
                        if (tex == nullptr) continue;

                        GreedyFaceKey key{};
                        key.tile = tile;
                        key.tex = tex;
                        key.mipmap = Tile::mipmapEnable[tileId];

                        uint32_t colorKey = 0;
                        int lightKey = 0;
                        computeFaceKey(tile, wx, wy, wz, face, colorKey,
                                       lightKey);
                        key.colorKey = colorKey;
                        key.lightKey = lightKey;

                        float ao[4];
                        computeFaceAO(tile, wx, wy, wz, face, ao);
                        auto aoByte = [](float v) -> uint32_t {
                            int iv = (int)(v * 255.0f + 0.5f);
                            if (iv < 0) iv = 0;
                            if (iv > 255) iv = 255;
                            return (uint32_t)iv;
                        };
                        key.aoKey = (aoByte(ao[0]) << 24) |
                                    (aoByte(ao[1]) << 16) |
                                    (aoByte(ao[2]) << 8) | aoByte(ao[3]);

                        valid[u + v * U] = 1;
                        keys[u + v * U] = key;
                    }
                }

                for (int v = 0; v < V; v++) {
                    for (int u = 0; u < U; u++) {
                        int idx = u + v * U;
                        if (!valid[idx]) continue;

                        GreedyFaceKey key = keys[idx];

                        int w = 1;
                        while (u + w < U && valid[idx + w] &&
                               keys[idx + w] == key) {
                            w++;
                        }

                        int h = 1;
                        bool done = false;
                        while (v + h < V && !done) {
                            for (int k = 0; k < w; k++) {
                                int idx2 = (u + k) + (v + h) * U;
                                if (!valid[idx2] || !(keys[idx2] == key)) {
                                    done = true;
                                    break;
                                }
                            }
                            if (!done) h++;
                        }

                        for (int dv = 0; dv < h; dv++) {
                            for (int du = 0; du < w; du++) {
                                valid[(u + du) + (v + dv) * U] = 0;
                            }
                        }

                        greedyMergedQuads++;
                        greedyTilesEmitted += (w * h);

                        beginGreedy();

                        t->setMipmapEnable(key.mipmap);
                        if (SharedConstants::TEXTURE_LIGHTING) {
                            t->tex2(key.lightKey);
                        }

                        float rr = ((key.colorKey >> 16) & 0xff) / 255.0f;
                        float gg = ((key.colorKey >> 8) & 0xff) / 255.0f;
                        float bb = (key.colorKey & 0xff) / 255.0f;

                        float ao1 = ((key.aoKey >> 24) & 0xff) / 255.0f;
                        float ao2 = ((key.aoKey >> 16) & 0xff) / 255.0f;
                        float ao3 = ((key.aoKey >> 8) & 0xff) / 255.0f;
                        float ao4 = (key.aoKey & 0xff) / 255.0f;

                        tileRenderer->setGreedyAO(
                            rr, gg, bb, ao1, ao2, ao3, ao4,
                            SharedConstants::TEXTURE_LIGHTING ? key.lightKey
                                                              : 0);

                        int baseX = 0, baseY = 0, baseZ = 0;
                        switch (face) {
                            case 0:
                            case 1:
                                baseX = x0 + u;
                                baseY = y0 + slice;
                                baseZ = z0 + v;
                                tileRenderer->setShape(
                                    0.0f, 0.0f, 0.0f, (float)w, 1.0f, (float)h);
                                if (face == 0) {
                                    t->normal(0.0f, -1.0f, 0.0f);
                                    tileRenderer->renderFaceDown(
                                        key.tile, baseX, baseY, baseZ, key.tex);
                                } else {
                                    t->normal(0.0f, 1.0f, 0.0f);
                                    tileRenderer->renderFaceUp(
                                        key.tile, baseX, baseY, baseZ, key.tex);
                                }
                                break;
                            case 2:
                            case 3:
                                baseX = x0 + u;
                                baseY = y0 + v;
                                baseZ = z0 + slice;
                                tileRenderer->setShape(
                                    0.0f, 0.0f, 0.0f, (float)w, (float)h, 1.0f);
                                if (face == 2) {
                                    t->normal(0.0f, 0.0f, -1.0f);
                                    tileRenderer->renderNorth(
                                        key.tile, baseX, baseY, baseZ, key.tex);
                                } else {
                                    t->normal(0.0f, 0.0f, 1.0f);
                                    tileRenderer->renderSouth(
                                        key.tile, baseX, baseY, baseZ, key.tex);
                                }
                                break;
                            case 4:
                            case 5:
                                baseX = x0 + slice;
                                baseY = y0 + v;
                                baseZ = z0 + u;
                                tileRenderer->setShape(0.0f, 0.0f, 0.0f, 1.0f,
                                                       (float)h, (float)w);
                                if (face == 4) {
                                    t->normal(-1.0f, 0.0f, 0.0f);
                                    tileRenderer->renderWest(
                                        key.tile, baseX, baseY, baseZ, key.tex);
                                } else {
                                    t->normal(1.0f, 0.0f, 0.0f);
                                    tileRenderer->renderEast(
                                        key.tile, baseX, baseY, baseZ, key.tex);
                                }
                                break;
                        }
                        // who the fuck at 4j named those functions.
                        tileRenderer->setApplyAmbienceOcclusion(false);
                        faceRendered = true;
                    }
                }
            }

            return faceRendered;
        };

        struct LiquidTopKey {
            Tile* tile;
            Icon* tex;
            uint32_t colorKey;
            int lightKey;
            bool mipmap;
            int heightQ;
            float height;
            bool operator==(const LiquidTopKey& other) const {
                return tile == other.tile && tex == other.tex &&
                       colorKey == other.colorKey &&
                       lightKey == other.lightKey && mipmap == other.mipmap &&
                       heightQ == other.heightQ;
            }
        };
        // liquids & oceans are usually flat, so they can definitively benefit from having this optimization.
        auto greedyMeshLiquidTop = [&]() {
            const int U = 16;
            const int V = 16;
            bool faceRendered = false;
            const float hEps = 1e-4f;

            for (int ly = 0; ly < 16; ly++) {
                std::vector<uint8_t> valid(U * V, 0);
                std::vector<LiquidTopKey> keys(U * V);

                for (int lz = 0; lz < V; lz++) {
                    for (int lx = 0; lx < U; lx++) {
                        unsigned char tileId = getTileIdLocal(lx, ly, lz);
                        if (tileId == 0 || tileId == 0xff) continue;

                        Tile* tile = Tile::tiles[tileId];
                        if (tile == nullptr) continue;
                        if (tile->getRenderShape() != Tile::SHAPE_WATER)
                            continue;
                        if (tile->getRenderLayer() != currentLayer) continue;

                        int wx = x0 + lx;
                        int wy = y0 + ly;
                        int wz = z0 + lz;

                        if (!tile->shouldRenderFace(level, wx, wy + 1, wz, 1))
                            continue;

                        if (LiquidTile::getSlopeAngle(level, wx, wy, wz,
                                                      tile->material) > -999)
                            continue;

                        float h0 = tileRenderer->getWaterHeightAt(
                            wx, wy, wz, tile->material);
                        float h1 = tileRenderer->getWaterHeightAt(
                            wx, wy, wz + 1, tile->material);
                        float h2 = tileRenderer->getWaterHeightAt(
                            wx + 1, wy, wz + 1, tile->material);
                        float h3 = tileRenderer->getWaterHeightAt(
                            wx + 1, wy, wz, tile->material);

                        float hmin = h0;
                        if (h1 < hmin) hmin = h1;
                        if (h2 < hmin) hmin = h2;
                        if (h3 < hmin) hmin = h3;
                        float hmax = h0;
                        if (h1 > hmax) hmax = h1;
                        if (h2 > hmax) hmax = h2;
                        if (h3 > hmax) hmax = h3;

                        if (hmax - hmin > hEps) continue;

                        float height = hmax - 0.001f;
                        if (height < 0.0f) height = hmax;
                        int heightQ = (int)(height * 4096.0f + 0.5f);

                        int data = level->getData(wx, wy, wz);
                        Icon* tex = tileRenderer->getTexture(tile, 1, data);
                        if (tex == nullptr) continue;

                        LiquidTopKey key{};
                        key.tile = tile;
                        key.tex = tex;
                        key.mipmap = Tile::mipmapEnable[tileId];
                        key.heightQ = heightQ;
                        key.height = height;

                        int col = tile->getColor(level, wx, wy, wz);
                        float r = ((col >> 16) & 0xff) / 255.0f;
                        float g = ((col >> 8) & 0xff) / 255.0f;
                        float b = ((col) & 0xff) / 255.0f;

                        if (GameRenderer::anaglyph3d) {
                            float cr = (r * 30 + g * 59 + b * 11) / 100;
                            float cg = (r * 30 + g * 70) / (100);
                            float cb = (r * 30 + b * 70) / (100);
                            r = cr;
                            g = cg;
                            b = cb;
                        }

                        if (SharedConstants::TEXTURE_LIGHTING) {
                            key.lightKey = tileRenderer->getLightColor(
                                tile, level, wx, wy, wz);
                        } else {
                            float br = tile->getBrightness(level, wx, wy, wz);
                            r *= br;
                            g *= br;
                            b *= br;
                            key.lightKey = 0;
                        }

                        auto clampByte = [](float v) -> uint8_t {
                            int iv = (int)(v * 255.0f + 0.5f);
                            if (iv < 0) iv = 0;
                            if (iv > 255) iv = 255;
                            return (uint8_t)iv;
                        };
                        key.colorKey = (clampByte(r) << 16) |
                                       (clampByte(g) << 8) | (clampByte(b));

                        valid[lx + lz * U] = 1;
                        keys[lx + lz * U] = key;
                    }
                }

                for (int v = 0; v < V; v++) {
                    for (int u = 0; u < U; u++) {
                        int idx = u + v * U;
                        if (!valid[idx]) continue;

                        LiquidTopKey key = keys[idx];

                        int w = 1;
                        while (u + w < U && valid[idx + w] &&
                               keys[idx + w] == key) {
                            w++;
                        }

                        int h = 1;
                        bool done = false;
                        while (v + h < V && !done) {
                            for (int k = 0; k < w; k++) {
                                int idx2 = (u + k) + (v + h) * U;
                                if (!valid[idx2] || !(keys[idx2] == key)) {
                                    done = true;
                                    break;
                                }
                            }
                            if (!done) h++;
                        }

                        for (int dv = 0; dv < h; dv++) {
                            for (int du = 0; du < w; du++) {
                                valid[(u + du) + (v + dv) * U] = 0;
                            }
                        }

                        greedyLiquidMergedQuads++;
                        greedyLiquidTilesEmitted += (w * h);

                        beginGreedy();
                        tileRenderer->setApplyAmbienceOcclusion(false);

                        t->setMipmapEnable(key.mipmap);
                        if (SharedConstants::TEXTURE_LIGHTING) {
                            t->tex2(key.lightKey);
                        }

                        float rr = ((key.colorKey >> 16) & 0xff) / 255.0f;
                        float gg = ((key.colorKey >> 8) & 0xff) / 255.0f;
                        float bb = (key.colorKey & 0xff) / 255.0f;
                        t->color(rr, gg, bb);

                        int baseX = x0 + u;
                        int baseY = y0 + ly;
                        int baseZ = z0 + v;
                        tileRenderer->setShape(0.0f, 0.0f, 0.0f, (float)w,
                                               key.height, (float)h);
                        t->normal(0.0f, 1.0f, 0.0f);
                        tileRenderer->renderFaceUp(key.tile, baseX, baseY,
                                                   baseZ, key.tex);

                        for (int dv = 0; dv < h; dv++) {
                            for (int du = 0; du < w; du++) {
                                int lx = u + du;
                                int lz = v + dv;
                                int maskIdx = (ly << 8) | (lz << 4) | lx;
                                greedyLiquidTop[maskIdx] = 1;
                            }
                        }

                        faceRendered = true;
                    }
                }
            }

            return faceRendered;
        };

#if defined(ENABLE_GREEDY_MESHING)
        rendered |= greedyMeshLiquidTop();

        if (currentLayer == 0) {
            tileRenderer->setApplyAmbienceOcclusion(false);
            rendered |= greedyMeshFace(0);
            rendered |= greedyMeshFace(1);
            rendered |= greedyMeshFace(2);
            rendered |= greedyMeshFace(3);
            rendered |= greedyMeshFace(4);
            rendered |= greedyMeshFace(5);
        }
        endGreedy();
#endif

        // 4J - changed loop order here to leave y as the innermost loop for
        // better cache performance
        for (int z = z0; z < z1; z++) {
            for (int x = x0; x < x1; x++) {
                for (int y = y0; y < y1; y++) {
                    // 4J Stu - tile data is ordered in 128 blocks of full
                    // width, lower 128 then upper 128
                    int indexY = y;
                    int offset = 0;
                    if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
                        indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
                        offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
                    }

                    // 4J - get tile from those copied into our local array in
                    // earlier optimisation
                    unsigned char tileId =
                        tileIds[offset +
                                (((x - x0) << 11) | ((z - z0) << 7) | indexY)];
                    // If flagged as not visible, drop out straight away
                    if (tileId == 0xff) continue;
                    //					int tileId =
                    // region->getTile(x,y,z);
                    if (tileId > 0) {
                        Tile* tile = Tile::tiles[tileId];
                        if (currentLayer == 0 && tile->isEntityTile()) {
                            std::shared_ptr<TileEntity> et =
                                region->getTileEntity(x, y, z);
                            if (TileEntityRenderDispatcher::instance
                                    ->hasRenderer(et)) {
                                renderableTileEntities.push_back(et);
                            }
                        }
                        int renderLayer = tile->getRenderLayer();

                        if (renderLayer != currentLayer) {
                            renderNextLayer = true;
                        } else if (renderLayer == currentLayer) {
#if defined(ENABLE_GREEDY_MESHING)
                            if (currentLayer == 0) {
                                int lx = x - x0;
                                int ly = y - y0;
                                int lz = z - z0;
                                if (greedyEligible[greedyIndex(lx, ly, lz)] !=
                                    0) {
                                    continue;
                                }
                            }
#endif

                            beginNormal();
                            rendered |=
                                tileRenderer->tesselateInWorld(tile, x, y, z);
                        }
                    }
                }
            }
        }

#if defined(ENABLE_GREEDY_MESHING)
        endGreedy();
#endif
        endNormal();

        if (listStarted) {
            glEndList();
            t->useCompactVertices(false);  // 4J added
        } else {
            rendered = false;
        }

        if (rendered) {
            levelRenderer->clearGlobalChunkFlag(
                this->x, this->y, this->z, level,
                LevelRenderer::CHUNK_FLAG_EMPTY0, currentLayer);
        } else {
            // 4J - added - clear any renderer data associated with this unused
            // list
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level,
                                              LevelRenderer::CHUNK_FLAG_EMPTY0,
                                              currentLayer);
            RenderManager.CBuffClear(lists + currentLayer);
        }
        if ((currentLayer == 0) && (!renderNextLayer)) {
            levelRenderer->setGlobalChunkFlag(this->x, this->y, this->z, level,
                                              LevelRenderer::CHUNK_FLAG_EMPTY1);
            RenderManager.CBuffClear(lists + 1);
            break;
        }
    }

    // 4J MGH - added this to take the bound from the value calc'd in the
    // tesselator
    bb = {bounds.boundingBox[0], bounds.boundingBox[1], bounds.boundingBox[2],
          bounds.boundingBox[3], bounds.boundingBox[4], bounds.boundingBox[5]};

    uint64_t conn = computeConnectivity(tileIds);  // pass tileIds
    int globalIdx =
        levelRenderer->getGlobalIndexForChunk(this->x, this->y, this->z, level);
    levelRenderer->setGlobalChunkConnectivity(globalIdx, conn);

    delete tileRenderer;
    delete region;

    // 4J - have rewritten the way that tile entities are stored globally to
    // make it work more easily with split screen. Chunks are now stored
    // globally in the levelrenderer, in a hashmap with a special key made up
    // from the dimension and chunk position (using same index as is used for
    // global flags)
    {
        std::lock_guard<std::mutex> lock(*globalRenderableTileEntities_cs);
        reconcileRenderableTileEntities(renderableTileEntities);
    }

    // 4J - These removed items are now also removed from
    // globalRenderableTileEntities

    if (LevelChunk::touchedSky) {
        levelRenderer->clearGlobalChunkFlag(
            x, y, z, level, LevelRenderer::CHUNK_FLAG_NOTSKYLIT);
    } else {
        levelRenderer->setGlobalChunkFlag(x, y, z, level,
                                          LevelRenderer::CHUNK_FLAG_NOTSKYLIT);
    }
    levelRenderer->setGlobalChunkFlag(x, y, z, level,
                                      LevelRenderer::CHUNK_FLAG_COMPILED);

    return;
}

float Chunk::distanceToSqr(std::shared_ptr<Entity> player) const {
    float xd = (float)(player->x - xm);
    float yd = (float)(player->y - ym);
    float zd = (float)(player->z - zm);
    return xd * xd + yd * yd + zd * zd;
}

float Chunk::squishedDistanceToSqr(std::shared_ptr<Entity> player) {
    float xd = (float)(player->x - xm);
    float yd = (float)(player->y - ym) * 2;
    float zd = (float)(player->z - zm);
    return xd * xd + yd * yd + zd * zd;
}

uint64_t Chunk::computeConnectivity(const uint8_t* tileIds) {
    const int W = 16;
    const int H = 16;
    const int VOLUME = W * H * W;

    auto idx = [&](int x, int y, int z) -> int {
        return y * W * W + z * W + x;
    };

    auto isOpen = [&](int lx, int ly, int lz) -> bool {
        int worldY = this->y + ly;
        int offset = 0;
        int indexY = worldY;
        if (indexY >= Level::COMPRESSED_CHUNK_SECTION_HEIGHT) {
            indexY -= Level::COMPRESSED_CHUNK_SECTION_HEIGHT;
            offset = Level::COMPRESSED_CHUNK_SECTION_TILES;
        }

        uint8_t tileId = tileIds[offset + ((lx << 11) | (lz << 7) | indexY)];

        if (tileId == 0) return true;      // air
        if (tileId == 0xFF) return false;  // hidden tile (yeah)

        Tile* t = Tile::tiles[tileId];
        return (t == nullptr) || !t->isSolidRender();
    };

    uint8_t visited[6][512];
    memset(visited, 0, sizeof(visited));

    static const int FX[6] = {1, -1, 0, 0, 0, 0};
    static const int FY[6] = {0, 0, 1, -1, 0, 0};
    static const int FZ[6] = {0, 0, 0, 0, 1, -1};

    struct Cell {
        int8_t x, y, z;
    };
    static thread_local std::vector<Cell> queue;

    uint64_t result = 0;

    for (int entryFace = 0; entryFace < 6; entryFace++) {
        uint8_t* vis = visited[entryFace];
        queue.clear();
        int x0s, x1s, y0s, y1s, z0s, z1s;
        switch (entryFace) {
            case 0:
                x0s = W - 1;
                x1s = W - 1;
                y0s = 0;
                y1s = H - 1;
                z0s = 0;
                z1s = W - 1;
                break;  // +X
            case 1:
                x0s = 0;
                x1s = 0;
                y0s = 0;
                y1s = H - 1;
                z0s = 0;
                z1s = W - 1;
                break;  // -X
            case 2:
                x0s = 0;
                x1s = W - 1;
                y0s = H - 1;
                y1s = H - 1;
                z0s = 0;
                z1s = W - 1;
                break;  // +Y
            case 3:
                x0s = 0;
                x1s = W - 1;
                y0s = 0;
                y1s = 0;
                z0s = 0;
                z1s = W - 1;
                break;  // -Y
            case 4:
                x0s = 0;
                x1s = W - 1;
                y0s = 0;
                y1s = H - 1;
                z0s = W - 1;
                z1s = W - 1;
                break;  // +Z
            case 5:
                x0s = 0;
                x1s = W - 1;
                y0s = 0;
                y1s = H - 1;
                z0s = 0;
                z1s = 0;
                break;  // -Z
            default:
                continue;
        }

        for (int sy = y0s; sy <= y1s; sy++)
            for (int sz = z0s; sz <= z1s; sz++)
                for (int sx = x0s; sx <= x1s; sx++) {
                    if (!isOpen(sx, sy, sz)) continue;
                    int i = idx(sx, sy, sz);
                    if (vis[i >> 3] & (1 << (i & 7))) continue;
                    vis[i >> 3] |= (1 << (i & 7));
                    queue.push_back({(int8_t)sx, (int8_t)sy, (int8_t)sz});
                }

        for (int qi = 0; qi < (int)queue.size(); qi++) {
            Cell cur = queue[qi];

            for (int nb = 0; nb < 6; nb++) {
                int nx = cur.x + FX[nb];
                int ny = cur.y + FY[nb];
                int nz = cur.z + FZ[nb];

                // entry exit conn
                if (nx < 0 || nx >= W || ny < 0 || ny >= H || nz < 0 ||
                    nz >= W) {
                    // nb IS the exit face because FX,FY,FZ are aligned
                    result |= ((uint64_t)1 << (entryFace * 6 + nb));
                    continue;
                }

                if (!isOpen(nx, ny, nz)) continue;

                int i = idx(nx, ny, nz);
                if (vis[i >> 3] & (1 << (i & 7))) continue;
                vis[i >> 3] |= (1 << (i & 7));
                queue.push_back({(int8_t)nx, (int8_t)ny, (int8_t)nz});
            }
        }
    }

    return result;
}
void Chunk::reset() {
    if (assigned) {
        int oldKey = -1;
        bool retireRenderableTileEntities = false;

        {
            std::lock_guard<std::recursive_mutex> lock(
                levelRenderer->m_csDirtyChunks);
            oldKey = levelRenderer->getGlobalIndexForChunk(x, y, z, level);
            unsigned char refCount =
                levelRenderer->decGlobalChunkRefCount(x, y, z, level);
            assigned = false;
            //		printf("\t\t [dec] refcount %d at %d, %d,
            //%d\n",refCount,x,y,z);
            if (refCount == 0 && oldKey != -1) {
                retireRenderableTileEntities = true;
                int lists = oldKey * 2;
                if (lists >= 0) {
                    lists += levelRenderer->chunkLists;
                    for (int i = 0; i < 2; i++) {
                        // 4J - added - clear any renderer data associated with
                        // this unused list
                        RenderManager.CBuffClear(lists + i);
                    }
                    levelRenderer->setGlobalChunkFlags(x, y, z, level, 0);
                }
            }
        }

        if (retireRenderableTileEntities) {
            levelRenderer->retireRenderableTileEntitiesForChunkKey(oldKey);
        }
    }

    clipChunk->visible = false;
}

void Chunk::_delete() {
    reset();
    level = nullptr;
}

int Chunk::getList(int layer) {
    if (!clipChunk->visible) return -1;

    int lists = levelRenderer->getGlobalIndexForChunk(x, y, z, level) * 2;
    lists += levelRenderer->chunkLists;

    bool empty = levelRenderer->getGlobalChunkFlag(
        x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, layer);
    if (!empty) return lists + layer;
    return -1;
}

void Chunk::cull(Culler* culler) {
    if (clipChunk->visible) {
        clipChunk->visible = culler->isVisible(&bb);
    }
}

void Chunk::renderBB() {
    //	glCallList(lists + 2);	// 4J - removed - TODO put back in
}

bool Chunk::isEmpty() {
    if (!levelRenderer->getGlobalChunkFlag(x, y, z, level,
                                           LevelRenderer::CHUNK_FLAG_COMPILED))
        return false;
    return levelRenderer->getGlobalChunkFlag(
        x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTYBOTH);
}

void Chunk::setDirty() {
    // 4J - not used, but if this starts being used again then we'll need to
    // investigate how best to handle it.
    __debugbreak();
    levelRenderer->setGlobalChunkFlag(x, y, z, level,
                                      LevelRenderer::CHUNK_FLAG_DIRTY);
}

void Chunk::clearDirty() {
    levelRenderer->clearGlobalChunkFlag(x, y, z, level,
                                        LevelRenderer::CHUNK_FLAG_DIRTY);
#if defined(_CRITICAL_CHUNKS)
    levelRenderer->clearGlobalChunkFlag(x, y, z, level,
                                        LevelRenderer::CHUNK_FLAG_CRITICAL);
#endif
}

bool Chunk::emptyFlagSet(int layer) {
    return levelRenderer->getGlobalChunkFlag(
        x, y, z, level, LevelRenderer::CHUNK_FLAG_EMPTY0, layer);
}
