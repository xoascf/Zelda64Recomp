#include "patches.h"
#include "transform_ids.h"
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"

#define CUE_DIST_THRESHOLD 10.0f

CsCmdActorCue* prev_cues_checked[ARRAY_COUNT(((CutsceneContext*)0)->actorCues)] = {0};

void Cutscene_ActorTranslate(Actor* actor, PlayState* play, s32 cueChannel) {
    Vec3f startPos;
    Vec3f endPos;
    CsCmdActorCue* cue = play->csCtx.actorCues[cueChannel];
    f32 lerp;

    startPos.x = cue->startPos.x;
    startPos.y = cue->startPos.y;
    startPos.z = cue->startPos.z;
    endPos.x = cue->endPos.x;
    endPos.y = cue->endPos.y;
    endPos.z = cue->endPos.z;

    lerp = Environment_LerpWeight(cue->endFrame, cue->startFrame, play->csCtx.curFrame);

    // @recomp Check if this is a new cue for a given channel.
    if (prev_cues_checked[cueChannel] != cue) {
        // New cue, so check if the start position of this cue is significantly different than the actor's current position.
        if (Math3D_Vec3f_DistXYZ(&startPos, &actor->world.pos) > CUE_DIST_THRESHOLD) {
            // The actor is probably being teleported, so skip interpolation for them this frame.
            actor_set_interpolation_skipped(actor);
        }
    }
    // @recomp Update tracking for this channel's most recent cue.
    prev_cues_checked[cueChannel] = cue;

    VEC3F_LERPIMPDST(&actor->world.pos, &startPos, &endPos, lerp);
}

extern EnHorseCsFunc D_808890F0[];
extern EnHorseCsFunc D_8088911C[];

// @recomp Patched to skip interpolation on Epona when she's teleported by a cutscene.
void func_80884718(EnHorse* this, PlayState* play) {
    CsCmdActorCue* cue;

    if (Cutscene_IsCueInChannel(play, CS_CMD_ACTOR_CUE_112)) {
        this->cueChannel = Cutscene_GetCueChannel(play, CS_CMD_ACTOR_CUE_112);
        cue = play->csCtx.actorCues[this->cueChannel];

        this->unk_1EC |= 0x20;
        if (this->cueId != cue->id) {
            if (this->cueId == -1) {
                // @recomp Being teleported by a new cue, so skip interpolation.
                actor_set_interpolation_skipped(&this->actor);

                this->actor.world.pos.x = cue->startPos.x;
                this->actor.world.pos.y = cue->startPos.y;
                this->actor.world.pos.z = cue->startPos.z;

                this->actor.world.rot.y = cue->rot.y;
                this->actor.shape.rot = this->actor.world.rot;

                this->actor.prevPos = this->actor.world.pos;
            }

            this->cueId = cue->id;
            // @recomp Manual relocation, TODO remove when automated.
            EnHorseCsFunc* D_808890F0_reloc = actor_relocate(&this->actor, D_808890F0);
            if (D_808890F0_reloc[this->cueId] != NULL) {
                D_808890F0_reloc[this->cueId](this, play, cue);
            }
        }

        // @recomp Manual relocation, TODO remove when automated.
        EnHorseCsFunc* D_8088911C_reloc = actor_relocate(&this->actor, D_8088911C);
        if (D_8088911C_reloc[this->cueId] != NULL) {
            D_8088911C_reloc[this->cueId](this, play, cue);
        }
    }
}

// @recomp Patched to skip interpolation on Epona when she's teleported by a cutscene.
void func_80883B70(EnHorse* this, CsCmdActorCue* cue) {
    // @recomp Being teleported by a new cue, so skip interpolation.
    actor_set_interpolation_skipped(&this->actor);

    this->actor.world.pos.x = cue->startPos.x;
    this->actor.world.pos.y = cue->startPos.y;
    this->actor.world.pos.z = cue->startPos.z;

    this->actor.world.rot.y = cue->rot.y;
    this->actor.shape.rot = this->actor.world.rot;

    this->actor.prevPos = this->actor.world.pos;
}

// @recomp Patched to skip interpolation on Link when he's teleported to a new cue.
void Player_Cutscene_SetPosAndYawToStart(Player* this, CsCmdActorCue* cue) {
    // @recomp Being teleported by a new cue, so skip interpolation.
    actor_set_interpolation_skipped(&this->actor);

    this->actor.world.pos.x = cue->startPos.x;
    this->actor.world.pos.y = cue->startPos.y;
    this->actor.world.pos.z = cue->startPos.z;

    this->currentYaw = this->actor.shape.rot.y = cue->rot.y;
}

CsCmdActorCue* prev_link_cue = NULL;

// @recomp Patched to skip interpolation on Link when he's teleported to a new cue.
void Player_Cutscene_Translate(PlayState* play, Player* this, CsCmdActorCue* cue) {
    f32 startX = cue->startPos.x;
    f32 startY = cue->startPos.y;
    f32 startZ = cue->startPos.z;
    f32 diffX = cue->endPos.x - startX;
    f32 diffY = cue->endPos.y - startY;
    f32 diffZ = cue->endPos.z - startZ;
    f32 progress = (((f32)(play->csCtx.curFrame - cue->startFrame)) / ((f32)(cue->endFrame - cue->startFrame)));

    // @recomp Check if this is a new cue for a given channel.
    if (prev_link_cue != cue) {
        // New cue, so check if the start position of this cue is significantly different than the actor's current position.
        Vec3f start_pos;
        start_pos.x = startX;
        start_pos.y = startY;
        start_pos.z = startZ;
        if (Math3D_Vec3f_DistXYZ(&start_pos, &this->actor.world.pos) > CUE_DIST_THRESHOLD) {
            // The actor is probably being teleported, so skip interpolation for them this frame.
            actor_set_interpolation_skipped(&this->actor);
        }
    }
    prev_link_cue = cue;

    this->actor.world.pos.x = (diffX * progress) + startX;
    this->actor.world.pos.y = (diffY * progress) + startY;
    this->actor.world.pos.z = (diffZ * progress) + startZ;
}
