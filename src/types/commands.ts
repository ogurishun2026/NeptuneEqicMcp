import type { ErrorResponse } from './errors.js';

export interface Command {
  id: string;
  cmd: string;
  params: Record<string, unknown>;
}

export interface CommandResponse {
  id: string;
  success: boolean;
  data?: Record<string, unknown>;
  error?: ErrorResponse;
}

export type Vector3 = [number, number, number];

// Actor Commands
export interface ActorCreateParams {
  name: string;
  location?: Vector3;
  rotation?: Vector3;
  scale?: Vector3;
  actorClass?: string;
}

export interface ActorDeleteParams {
  actorId?: string;
  name?: string;
}

export interface ActorFindParams {
  byId?: string;
  byName?: string;
  byClass?: string;
  byTag?: string;
}

export interface ActorTransformParams {
  actorId: string;
  location?: Vector3;
  rotation?: Vector3;
  scale?: Vector3;
}

export interface ActorPropertyParams {
  actorId: string;
  propertyName: string;
  propertyType: 'int' | 'float' | 'string' | 'bool' | 'vector' | 'rotator' | 'color';
  value: unknown;
}

export interface ActorComponentParams {
  actorId: string;
  componentType: string;
  name?: string;
}

export interface ActorRemoveComponentParams {
  actorId: string;
  componentId: string;
}
