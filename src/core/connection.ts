import WebSocket from 'ws';
import { EventEmitter } from 'events';
import { Command, CommandResponse } from '../types/index.js';
import { v4 as uuidv4 } from 'uuid';

export type ConnectionState = 'connecting' | 'connected' | 'disconnecting' | 'disconnected';

export interface ConnectionOptions {
  host: string;
  port: number;
  reconnectInterval?: number;
  commandTimeout?: number;
}

export class ConnectionManager extends EventEmitter {
  private ws: WebSocket | null = null;
  private state: ConnectionState = 'disconnected';
  private readonly options: Required<ConnectionOptions>;
  private pendingCommands: Map<string, {
    resolve: (response: CommandResponse) => void;
    reject: (error: Error) => void;
    timeout: NodeJS.Timeout;
  }> = new Map();

  constructor(options: ConnectionOptions) {
    super();
    this.options = {
      reconnectInterval: 1000,
      commandTimeout: 30000,
      ...options,
    };
  }

  getState(): ConnectionState {
    return this.state;
  }

  isConnected(): boolean {
    return this.state === 'connected' && this.ws?.readyState === WebSocket.OPEN;
  }

  async connect(): Promise<void> {
    if (this.isConnected()) return;

    this.setState('connecting');

    return new Promise((resolve, reject) => {
      const url = `ws://${this.options.host}:${this.options.port}`;
      this.ws = new WebSocket(url);

      this.ws.on('open', () => {
        this.setState('connected');
        resolve();
      });

      this.ws.on('message', (data: Buffer) => {
        this.handleMessage(data);
      });

      this.ws.on('close', () => {
        this.setState('disconnected');
        this.rejectAllPending('Connection closed');
      });

      this.ws.on('error', (error: Error) => {
        if (this.state === 'connecting') {
          reject(error);
        }
        this.setState('disconnected');
      });
    });
  }

  async disconnect(): Promise<void> {
    if (!this.ws) return;

    this.setState('disconnecting');
    this.ws.close();
    this.ws = null;
    this.setState('disconnected');
  }

  async sendCommand(cmd: string, params: Record<string, unknown>): Promise<CommandResponse> {
    if (!this.isConnected()) {
      await this.connect();
    }

    const id = uuidv4();
    const command: Command = { id, cmd, params };

    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.pendingCommands.delete(id);
        reject(new Error(`Command timeout: ${cmd}`));
      }, this.options.commandTimeout);

      this.pendingCommands.set(id, { resolve, reject, timeout });

      this.ws!.send(JSON.stringify(command));
    });
  }

  private handleMessage(data: Buffer): void {
    try {
      const message = JSON.parse(data.toString());

      // Check if it's a command response
      if (message.id !== undefined) {
        const pending = this.pendingCommands.get(message.id);
        if (pending) {
          clearTimeout(pending.timeout);
          this.pendingCommands.delete(message.id);
          pending.resolve(message as CommandResponse);
        }
      } else if (message.event !== undefined) {
        // It's an event from Unreal
        this.emit('event', message);
      }
    } catch (error) {
      this.emit('error', error);
    }
  }

  private setState(state: ConnectionState): void {
    this.state = state;
    this.emit('stateChange', state);
  }

  private rejectAllPending(reason: string): void {
    for (const [id, pending] of this.pendingCommands) {
      clearTimeout(pending.timeout);
      pending.reject(new Error(reason));
    }
    this.pendingCommands.clear();
  }
}
