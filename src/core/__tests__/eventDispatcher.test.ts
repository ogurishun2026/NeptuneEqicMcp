import { describe, it, expect, vi } from 'vitest';
import { EventDispatcher } from '../eventDispatcher.js';
import { UnrealEvent, EventTypes } from '../../types/index.js';

describe('EventDispatcher', () => {
  it('should dispatch events to registered handlers', () => {
    const dispatcher = new EventDispatcher();
    const handler = vi.fn();

    dispatcher.on(EventTypes.ACTOR_DELETED, handler);

    const event: UnrealEvent = {
      event: EventTypes.ACTOR_DELETED,
      timestamp: '2026-04-27T10:00:00Z',
      data: { actorId: '123' },
    };

    dispatcher.dispatch(event);
    expect(handler).toHaveBeenCalledWith(event);
  });

  it('should support wildcard handlers', () => {
    const dispatcher = new EventDispatcher();
    const handler = vi.fn();

    dispatcher.on('*', handler);

    const event: UnrealEvent = {
      event: EventTypes.ACTOR_CREATED,
      timestamp: '2026-04-27T10:00:00Z',
      data: { actorId: '123' },
    };

    dispatcher.dispatch(event);
    expect(handler).toHaveBeenCalledWith(event);
  });

  it('should allow unsubscribing', () => {
    const dispatcher = new EventDispatcher();
    const handler = vi.fn();

    const unsubscribe = dispatcher.on(EventTypes.ACTOR_DELETED, handler);
    unsubscribe();

    const event: UnrealEvent = {
      event: EventTypes.ACTOR_DELETED,
      timestamp: '2026-04-27T10:00:00Z',
      data: { actorId: '123' },
    };

    dispatcher.dispatch(event);
    expect(handler).not.toHaveBeenCalled();
  });
});
