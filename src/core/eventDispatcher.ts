import { UnrealEvent, EventHandler } from '../types/index.js';

type Unsubscribe = () => void;

export class EventDispatcher {
  private handlers: Map<string, Set<EventHandler>> = new Map();

  on(eventType: string, handler: EventHandler): Unsubscribe {
    if (!this.handlers.has(eventType)) {
      this.handlers.set(eventType, new Set());
    }
    this.handlers.get(eventType)!.add(handler);

    return () => {
      this.handlers.get(eventType)?.delete(handler);
    };
  }

  dispatch(event: UnrealEvent): void {
    // Dispatch to specific handlers
    const specificHandlers = this.handlers.get(event.event);
    if (specificHandlers) {
      for (const handler of specificHandlers) {
        handler(event);
      }
    }

    // Dispatch to wildcard handlers
    const wildcardHandlers = this.handlers.get('*');
    if (wildcardHandlers) {
      for (const handler of wildcardHandlers) {
        handler(event);
      }
    }
  }

  clear(): void {
    this.handlers.clear();
  }
}
