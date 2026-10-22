        case 10:
        {
            if (cmdbuf[0] != IPC_MakeHeader(10, 0, 2))
            {
                error(cmdbuf, 0xD9001830);
                break;
            }

            char *path = (char *)cmdbuf[2];
            strncpy(path, ctx->pluginPath, 255);

            cmdbuf[0] = IPC_MakeHeader(10, 1, 2);
            cmdbuf[1] = 0;
            cmdbuf[2] = IPC_Desc_Buffer(255, IPC_BUFFER_RW);
            cmdbuf[3] = (u32)path;

            break;
        }
