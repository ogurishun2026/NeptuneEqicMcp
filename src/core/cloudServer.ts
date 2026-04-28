import WebSocket, { WebSocketServer } from 'ws';
import { EventEmitter } from 'events';
import { Command, CommandResponse } from '../types/index.js';
import { v4 as uuidv4 } from 'uuid';

export type ServerState = 'starting' | 'running' | 'stopping' | 'stopped';

export interface CloudServerOptions {
  port: number;
  commandTimeout?: number;
}

/**
 * CloudServer - 云端模式下的 WebSocket 服务器
 *
 * 用于云端部署，等待 Unreal 客户端连接。
 */
export class CloudServer extends EventEmitter {
  private wss: WebSocketServer | null = null;
  private state: ServerState = 'stopped';
  private readonly options: Required<CloudServerOptions>;
  private connectedClient: WebSocket | null = null;
  private pendingCommands: Map<string, {
    resolve: (response: CommandResponse) => void;
    reject: (error: Error) => void;
    timeout: NodeJS.Timeout;
  }> = new Map();

  constructor(options: CloudServerOptions) {
    super();
    this.options = {
      commandTimeout: 30000,
      ...options,
    };
  }

  getState(): ServerState {
    return this.state;
  }

  isRunning(): boolean {
    return this.state === 'running';
  }

  hasClient(): boolean {
    return this.connectedClient !== null && this.connectedClient.readyState === WebSocket.OPEN;
  }

  /**
   * 启动服务器，等待 Unreal 连接
   */
  async start(): Promise<void> {
    if (this.isRunning()) return;

    this.state = 'starting';

    return new Promise((resolve, reject) => {
      this.wss = new WebSocketServer({ port: this.options.port });

      this.wss.on('listening', () => {
        this.state = 'running';
        console.error(`Cloud server started on port ${this.options.port}`);
        this.emit('started');
        resolve();
      });

      this.wss.on('connection', (ws: WebSocket) => {
        if (this.connectedClient) {
          // 已经有客户端连接，拒绝新连接
          ws.close(1000, 'Another client already connected');
          return;
        }

        this.connectedClient = ws;
        console.error('Unreal client connected');
        this.emit('clientConnected');

        ws.on('message', (data: Buffer) => {
          this.handleMessage(data);
        });

        ws.on('close', () => {
          console.error('Unreal client disconnected');
          this.connectedClient = null;
          this.rejectAllPending('Client disconnected');
          this.emit('clientDisconnected');
        });

        ws.on('error', (error: Error) => {
          console.error('Client error:', error.message);
          this.emit('error', error);
        });
      });

      this.wss.on('error', (error: Error) => {
        if (this.state === 'starting') {
          reject(error);
        }
        this.emit('error', error);
      });
    });
  }

  /**
   * 停止服务器
   */
  async stop(): Promise<void> {
    if (!this.wss) return;

    this.state = 'stopping';

    if (this.connectedClient) {
      this.connectedClient.close();
      this.connectedClient = null;
    }

    this.rejectAllPending('Server stopping');

    return new Promise((resolve) => {
      this.wss!.close(() => {
        this.state = 'stopped';
        console.error('Cloud server stopped');
        this.emit('stopped');
        resolve();
      });
    });
  }

  /**
   * 发送命令到 Unreal
   */
  async sendCommand(cmd: string, params: Record<string, unknown>): Promise<CommandResponse> {
    if (!this.hasClient()) {
      throw new Error('No Unreal client connected');
    }

    const id = uuidv4();
    const command: Command = { id, cmd, params };

    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.pendingCommands.delete(id);
        reject(new Error(`Command timeout: ${cmd}`));
      }, this.options.commandTimeout);

      this.pendingCommands.set(id, { resolve, reject, timeout });

      this.connectedClient!.send(JSON.stringify(command));
    });
  }

  private handleMessage(data: Buffer): void {
    try {
      const message = JSON.parse(data.toString());

      // 命令响应
      if (message.id !== undefined) {
        const pending = this.pendingCommands.get(message.id);
        if (pending) {
          clearTimeout(pending.timeout);
          this.pendingCommands.delete(message.id);
          pending.resolve(message as CommandResponse);
        }
      } else if (message.event !== undefined) {
        // Unreal 发来的事件
        this.emit('event', message);
      }
    } catch (error) {
      this.emit('error', error);
    }
  }

  private rejectAllPending(reason: string): void {
    for (const [id, pending] of this.pendingCommands) {
      clearTimeout(pending.timeout);
      pending.reject(new Error(reason));
    }
    this.pendingCommands.clear();
  }
}
