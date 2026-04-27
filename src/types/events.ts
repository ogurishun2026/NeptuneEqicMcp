export interface UnrealEvent {
  event: string;
  timestamp: string;
  data: Record<string, unknown>;
}

export type EventHandler = (event: UnrealEvent) => void;

export const EventTypes = {
  ACTOR_CREATED: 'actor.created',
  ACTOR_DELETED: 'actor.deleted',
  ACTOR_MODIFIED: 'actor.modified',
  PLAY_MODE_STARTED: 'play_mode.started',
  PLAY_MODE_ENDED: 'play_mode.ended',
  ASSET_IMPORTED: 'asset.imported',
  ERROR: 'error',
} as const;
