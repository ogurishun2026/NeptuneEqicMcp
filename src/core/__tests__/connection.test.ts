import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { ConnectionManager } from '../connection.js';

describe('ConnectionManager', () => {
  let manager: ConnectionManager;

  beforeEach(() => {
    manager = new ConnectionManager({ host: 'localhost', port: 18765 });
  });

  afterEach(async () => {
    await manager.disconnect();
  });

  it('should start in disconnected state', () => {
    expect(manager.isConnected()).toBe(false);
  });

  it('should emit state change events', () => {
    const listener = vi.fn();
    manager.on('stateChange', listener);
    manager.emit('stateChange', 'connected');
    expect(listener).toHaveBeenCalledWith('connected');
  });
});
