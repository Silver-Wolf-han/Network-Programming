if (info.op[i] == PIPE) {
    /*
    int fd[2];

    if (pipe(fd) < 0) {
        cerr << "Error: Unable to create pipe\n";
        exit(1);
    }
    */
    pid_t pid = fork();
    if (pid < 0) {
        cerr << "Error: Unable to fork\n";
        exit(1);
    } else if (pid == 0) {
        /*
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        */
        //close(pipeList[currentCommandStart + i].fd[0]);
        dup2(pipeList[currentCommandStart + i].fd[1], STDOUT_FILENO);
        //close(pipeList[currentCommandStart + i].fd[1]);
        
        for (size_t j = 0; j < (size_t)(currentCommandStart + i); ++j) {
            if (pipeList[j].OutCommandIndex == currentCommandStart + (int)i) {
                //close(pipeList[j].fd[1]);
                dup2(pipeList[j].fd[0], STDIN_FILENO);
                //close(pipeList[j].fd[0]);
            }
        }
        /*
        if (i == 0) {
            // first command input pipe handle
            for (size_t j = 0; j < (size_t)currentCommandStart; ++j) {
                if (pipeList[j].OutCommandIndex == currentCommandStart) {
                    close(pipeList[j].fd[1]);
                    dup2(pipeList[j].fd[0], STDIN_FILENO);
                    close(pipeList[j].fd[0]);
                }
            }
        }
        */
        for (size_t j = 0; j < pipeList.size(); ++j) {
            close(pipeList[j].fd[0]);
            close(pipeList[j].fd[1]);
        }

        vector<char*> args;
        for (auto &arg : info.argv[i]) {
            args.push_back(arg.data());
        }
        args.push_back(nullptr);

        if (execvp(args[0], args.data()) == -1) {
            cerr << "Unknown command: [" << args[0] << "]\n";
            exit(1);
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
        /*
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        */
        /*
        for (size_t j = 0; j < (size_t)(currentCommandStart + i+1); ++j) {
            if (pipeList[j].OutCommandIndex == currentCommandStart + (int)(i+1)) {
                close(pipeList[j].fd[1]);
                dup2(pipeList[j].fd[0], STDIN_FILENO);
                close(pipeList[j].fd[0]);
            }
        }
        */
    }
} else {

    int fd;
    if (info.op[i] == OUT_RD) {
        fd = open(info.argv[i+1][0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd < 0) {
            cerr << "Error: " << info.argv[i+1][0] << ": Failed to open or create file\n";
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    for (size_t j = 0; j < (size_t)(currentCommandStart + i); ++j) {
        if (pipeList[j].OutCommandIndex == currentCommandStart + (int)i) {
            //close(pipeList[j].fd[1]);
            dup2(pipeList[j].fd[0], STDIN_FILENO);
            //close(pipeList[j].fd[0]);
        }
    }
    /*
    if (i == 0) {
        // first command input pipe handle
        for (size_t j = 0; j < (size_t)currentCommandStart; ++j) {
            if (pipeList[j].OutCommandIndex == currentCommandStart) {
                close(pipeList[j].fd[1]);
                dup2(pipeList[j].fd[0], STDIN_FILENO);
                close(pipeList[j].fd[0]);
            }
        }
    }
    */
    //close(pipeList[currentCommandStart+i].fd[0]);
    //close(pipeList[currentCommandStart+i].fd[1]);

    
    for (size_t j = 0; j < pipeList.size(); ++j) {
        close(pipeList[j].fd[0]);
        close(pipeList[j].fd[1]);
    }
    
    vector<char*> args;
    for (auto &arg : info.argv[i]) {
        args.push_back(arg.data());
    }
    args.push_back(nullptr);

    if (execvp(args[0], args.data()) == -1) {
        cerr << "Unknown command: [" << args[0] << "]\n";
        exit(1);
    }
}