export interface ErrorResponse {
  code: string;
  type: ErrorType;
  message: string;
  stack?: string;
}

export type ErrorType =
  | 'validation_error'
  | 'not_found'
  | 'runtime_error'
  | 'connection_error'
  | 'timeout_error'
  | 'permission_error';

export const ErrorCodes = {
  ACTOR_NOT_FOUND: 'ACTOR_NOT_FOUND',
  ACTOR_SPAWN_FAILED: 'ACTOR_SPAWN_FAILED',
  ACTOR_DELETE_FAILED: 'ACTOR_DELETE_FAILED',
  INVALID_PARAMS: 'INVALID_PARAMS',
  CONNECTION_FAILED: 'CONNECTION_FAILED',
  COMMAND_TIMEOUT: 'COMMAND_TIMEOUT',
  UNKNOWN_COMMAND: 'UNKNOWN_COMMAND',
} as const;

export function createError(
  code: string,
  type: ErrorType,
  message: string,
  stack?: string
): ErrorResponse {
  return { code, type, message, stack };
}
