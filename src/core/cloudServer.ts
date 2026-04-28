import { createServer, Socket, Server as NetServer } from 'net';
import { createHash } from 'crypto';
import { EventEmitter } from 'events';
import { Command, CommandResponse } from '../types/index.js';
import { v4 as uuidv4 } from 'uuid';

export type ServerState = 'starting' | 'running' | 'stopping' | 'stopped';

export interface CloudServerOptions {
  port: number;
  commandTimeout?: number;
}

/**
 * WebSocket 帧解析器 - 兼容没有 MASK 的客户端
 */
class WebSocketFrameParser {
  private buffer: Buffer = Buffer.alloc(0);

  append(data: Buffer): void {
    this.buffer = Buffer.concat([this.buffer, data]);
  }

  /**
   * 解析帧，跳过 MASK 验证
   */
  parse(): { payload: Buffer; fin: boolean; opcode: number } | null {
    if (this.buffer.length < 2) return null;

    const firstByte = this.buffer[0];
    const secondByte = this.buffer[1];

    const fin = (firstByte & 0x80) !== 0;
    const opcode = firstByte & 0x0f;
    const masked = (secondByte & 0x80) !== 0;
    let payloadLength = secondByte & 0x7f;

    let offset = 2;

    if (payloadLength === 126) {
      if (this.buffer.length < 4) return null;
      payloadLength = this.buffer.readUInt16BE(2);
      offset = 4;
    } else if (payloadLength === 127) {
      if (this.buffer.length < 10) return null;
      payloadLength = Number(this.buffer.readBigUInt64BE(2));
      offset = 10;
    }

    // 计算总帧长度
    const maskKeyLength = masked ? 4 : 0;
    const totalLength = offset + maskKeyLength + payloadLength;

    if (this.buffer.length < totalLength) return null;

    // 提取 payload
    let payload = this.buffer.subarray(offset + maskKeyLength, offset + maskKeyLength + payloadLength);

    // 如果有 mask，解码
    if (masked && maskKeyLength === 4) {
      const maskKey = this.buffer.subarray(offset, offset + 4);
      const decoded = Buffer.alloc(payloadLength);
      for (let i = 0; i < payloadLength; i++) {
        decoded[i] = payload[i] ^ maskKey[i % 4];
      }
      payload = decoded;
    }

    // 移除已处理的数据
    this.buffer = this.buffer.subarray(totalLength);

    return { payload, fin, opcode };
  }
}

/**
 * CloudServer - 云端模式下的 WebSocket 服务器
 * 兼容 UE 的 WebSocket 实现（可能缺少 MASK）
 */
export class CloudServer extends EventEmitter {
  private server: NetServer | null = null;
  private state: ServerState = 'stopped';
  private readonly options: Required<CloudServerOptions>;
  private connectedSocket: Socket | null = null;
  private parser: WebSocketFrameParser | null = null;
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
    return this.connectedSocket !== null;
  }

  /**
   * 启动服务器，等待 Unreal 连接
   */
  async start(): Promise<void> {
    if (this.isRunning()) return;

    this.state = 'starting';

    return new Promise((resolve, reject) => {
      this.server = createServer((socket) => {
        if (this.connectedSocket) {
          socket.destroy();
          return;
        }

        console.error('Client connecting...');
        this.handleNewConnection(socket);
      });

      this.server.listen(this.options.port, () => {
        this.state = 'running';
        console.error(`Cloud server started on port ${this.options.port}`);
        this.emit('started');
        resolve();
      });

      this.server.on('error', (error) => {
        if (this.state === 'starting') {
          reject(error);
        }
        this.emit('error', error);
      });
    });
  }

  private handleNewConnection(socket: Socket): void {
    let handshaken = false;

    socket.once('data', (data) => {
      const request = data.toString();
      if (request.includes('Upgrade: websocket')) {
        const keyMatch = request.match(/Sec-WebSocket-Key: (.+)\r\n/);
        const key = keyMatch ? keyMatch[1].trim() : '';

        // 生成 accept key
        const accept = createHash('sha1')
          .update(key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11')
          .digest('base64');

        const response = [
          'HTTP/1.1 101 Switching Protocols',
          'Upgrade: websocket',
          'Connection: Upgrade',
          `Sec-WebSocket-Accept: ${accept}`,
          '\r\n'
        ].join('\r\n');

        socket.write(response);
        handshaken = true;

        this.connectedSocket = socket;
        this.parser = new WebSocketFrameParser();

        console.error('Unreal client connected');
        this.emit('clientConnected');

        // 继续处理剩余数据（可能有）
        socket.on('data', (data) => this.handleData(data));
      }
    });

    socket.on('close', () => {
      if (handshaken) {
        console.error('Unreal client disconnected');
        this.emit('clientDisconnected');
      }
      this.connectedSocket = null;
      this.parser = null;
      this.rejectAllPending('Client disconnected');
    });

    socket.on('error', (error) => {
      console.error('Socket error:', error.message);
      this.emit('error', error);
    });
  }

  private handleData(data: Buffer): void {
    if (!this.parser) return;

    this.parser.append(data);

    while (true) {
      const result = this.parser.parse();
      if (!result) break;

      // 只处理文本帧
      if (result.opcode === 1 && result.fin) {
        try {
          const message = JSON.parse(result.payload.toString());
          this.handleMessage(message);
        } catch (e) {
          console.error('Failed to parse message:', e);
        }
      }
      // 处理关闭帧
      else if (result.opcode === 8) {
        this.connectedSocket?.end();
      }
    }
  }

  /**
   * 停止服务器
   */
  async stop(): Promise<void> {
    if (!this.server) return;

    this.state = 'stopping';

    if (this.connectedSocket) {
      this.connectedSocket.destroy();
      this.connectedSocket = null;
    }

    this.rejectAllPending('Server stopping');

    return new Promise((resolve) => {
      this.server!.close(() => {
        this.state = 'stopped';
        console.error('Cloud server stopped');
        this.emit('stopped');
        resolve();
      });
    });
  }

  /**
   * 发送 WebSocket 文本帧
   */
  private sendFrame(data: Buffer): void {
    if (!this.connectedSocket) return;

    const length = data.length;
    let frame: Buffer;

    if (length <= 125) {
      frame = Buffer.alloc(2 + length);
      frame[0] = 0x81; // FIN + Text frame
      frame[1] = length; // 服务器发送不需要 MASK
      data.copy(frame, 2);
    } else if (length <= 65535) {
      frame = Buffer.alloc(4 + length);
      frame[0] = 0x81;
      frame[1] = 126;
      frame.writeUInt16BE(length, 2);
      data.copy(frame, 4);
    } else {
      frame = Buffer.alloc(10 + length);
      frame[0] = 0x81;
      frame[1] = 127;
      frame.writeBigUInt64BE(BigInt(length), 2);
      data.copy(frame, 10);
    }

    this.connectedSocket.write(frame);
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
      this.sendFrame(Buffer.from(JSON.stringify(command)));
    });
  }

  private handleMessage(message: any): void {
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
  }

  private rejectAllPending(reason: string): void {
    for (const [, pending] of this.pendingCommands) {
      clearTimeout(pending.timeout);
      pending.reject(new Error(reason));
    }
    this.pendingCommands.clear();
  }
}
