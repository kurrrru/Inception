const BOARD_SIZE = 8;
const HUMAN_RESULT = {
    INVALID: 0,
    OK: 1,
    FORCED_PASS: 2,
    NOT_TURN: 3,
    GAME_OVER: 4,
};

const state = {
    module: null,
    api: null,
    humanColor: 0,
    boardLocked: true,
};

const elements = {
    board: document.getElementById('board'),
    colorSelect: document.getElementById('color-select'),
    newGame: document.getElementById('new-game'),
    pass: document.getElementById('pass-button'),
    scoreBlack: document.getElementById('score-black'),
    scoreWhite: document.getElementById('score-white'),
    status: document.getElementById('status'),
};

const cells = [];

// Ensure the board is a square sized to fit the viewport on mobile and desktop.
function adjustBoardSize() {
    const boardEl = document.getElementById('board');
    if (!boardEl) return;
    // Reserve some vertical space for header/footer and side panel. Estimate 180px.
    const reservedV = 180;
    const maxPx = 520; // same as CSS max
    const vw = Math.max(document.documentElement.clientWidth || 0, window.innerWidth || 0);
    const vh = Math.max(document.documentElement.clientHeight || 0, window.innerHeight || 0);
    // choose the largest square that fits in viewport while leaving reserved space
    const candidate = Math.min(vw * 0.9, vh - reservedV, maxPx);
    const size = Math.max(120, Math.floor(candidate)); // ensure a minimum playable size
    boardEl.style.width = size + 'px';
    boardEl.style.height = size + 'px';
}

function toCoordinate(row, col) {
    const colChar = String.fromCharCode('A'.charCodeAt(0) + col);
    return `${colChar}${row + 1}`;
}

function setStatus(message) {
    elements.status.textContent = message;
}

function buildBoard() {
    for (let row = 0; row < BOARD_SIZE; row += 1) {
        const rowCells = [];
        for (let col = 0; col < BOARD_SIZE; col += 1) {
            const button = document.createElement('button');
            button.className = 'cell';
            button.type = 'button';
            button.dataset.row = row;
            button.dataset.col = col;
            button.addEventListener('click', () => handleCellClick(row, col));
            elements.board.appendChild(button);
            rowCells.push(button);
        }
        cells.push(rowCells);
    }
}

function bindApi(Module) {
    return {
        webReset: Module.cwrap('web_reset', null, ['number']),
        webGetCell: Module.cwrap('web_get_cell', 'number', ['number', 'number']),
        webGetCurrentPlayer: Module.cwrap('web_get_current_player', 'number', []),
        webGetHumanPlayer: Module.cwrap('web_get_human_player', 'number', []),
        webIsGameOver: Module.cwrap('web_is_game_over', 'number', []),
        webGetBlackCount: Module.cwrap('web_get_black_count', 'number', []),
        webGetWhiteCount: Module.cwrap('web_get_white_count', 'number', []),
        webHasAnyMove: Module.cwrap('web_has_any_move', 'number', ['number']),
        webIsValidMove: Module.cwrap('web_is_valid_move', 'number', ['number', 'number', 'number']),
        webHumanMove: Module.cwrap('web_human_move', 'number', ['number', 'number']),
        webHumanPass: Module.cwrap('web_human_pass', 'number', []),
        webAiMove: Module.cwrap('web_ai_move', 'number', []),
        webGetWinner: Module.cwrap('web_get_winner', 'number', []),
    };
}

function updateScores() {
    elements.scoreBlack.textContent = state.api.webGetBlackCount();
    elements.scoreWhite.textContent = state.api.webGetWhiteCount();
}

function renderBoard() {
    if (!state.api) {
        return;
    }
    const isGameOver = state.api.webIsGameOver() === 1;
    const currentPlayer = state.api.webGetCurrentPlayer();
    const humanTurn = currentPlayer === state.humanColor && !isGameOver;
    const canInteract = humanTurn && !state.boardLocked;
    const showHints = canInteract && state.api.webHasAnyMove(state.humanColor) === 1;

    for (let row = 0; row < BOARD_SIZE; row += 1) {
        for (let col = 0; col < BOARD_SIZE; col += 1) {
            const cell = cells[row][col];
            const value = state.api.webGetCell(row, col);
            cell.classList.toggle('black', value === 1);
            cell.classList.toggle('white', value === 2);
            cell.classList.toggle('valid', showHints && state.api.webIsValidMove(row, col, state.humanColor) === 1);
            const disabled = !canInteract;
            cell.classList.toggle('disabled', disabled);
            cell.disabled = disabled;
        }
    }
    elements.board.classList.toggle('disabled', !canInteract);
    elements.pass.disabled = !canInteract;
}

function announceWinner() {
    const winner = state.api.webGetWinner();
    if (winner === 0) {
        setStatus('対局終了：黒の勝ちです。');
    } else if (winner === 1) {
        setStatus('対局終了：白の勝ちです。');
    } else if (winner === 2) {
        setStatus('対局終了：引き分けです。');
    } else {
        setStatus('対局が終了しました。');
    }
    state.boardLocked = true;
    renderBoard();
}

function queueAiMove() {
    state.boardLocked = true;
    renderBoard();
    setStatus('AIが考えています...');
    window.setTimeout(() => {
        const result = state.api.webAiMove();
        handleAiMoveResult(result);
    }, 120);
}

function handleAiMoveResult(result) {
    if (result === -3) {
        announceWinner();
        return;
    }
    if (result === -2) {
        // AI が呼ばれるタイミングがずれた場合
        setStatus('AIの手番ではありません。');
        state.boardLocked = false;
        renderBoard();
        return;
    }
    if (result === -1) {
        setStatus('AIはパスしました。');
    } else if (result >= 0) {
        const row = Math.floor(result / BOARD_SIZE);
        const col = result % BOARD_SIZE;
        setStatus(`AIが ${toCoordinate(row, col)} に打ちました。`);
    }
    renderBoard();
    updateScores();
    if (state.api.webIsGameOver() === 1) {
        announceWinner();
        return;
    }
    unlockForHuman();
}

function unlockForHuman() {
    if (state.api.webIsGameOver() === 1) {
        announceWinner();
        return;
    }
    const hasMove = state.api.webHasAnyMove(state.humanColor) === 1;
    if (!hasMove) {
        const passResult = state.api.webHumanPass();
        if (passResult === HUMAN_RESULT.FORCED_PASS) {
            setStatus('あなたに合法手がないため自動的にパスしました。');
            renderBoard();
            updateScores();
            if (state.api.webIsGameOver() === 1) {
                announceWinner();
            } else {
                queueAiMove();
            }
        } else if (passResult === HUMAN_RESULT.GAME_OVER) {
            announceWinner();
        } else {
            setStatus('パスできませんでした。');
        }
        return;
    }
    state.boardLocked = false;
    renderBoard();
    setStatus('あなたの番です。');
}

function handleCellClick(row, col) {
    if (!state.api || state.boardLocked) {
        return;
    }
    const result = state.api.webHumanMove(row, col);
    if (result === HUMAN_RESULT.GAME_OVER) {
        renderBoard();
        updateScores();
        announceWinner();
        return;
    }
    if (result === HUMAN_RESULT.NOT_TURN) {
        setStatus('まだAIの番です。');
        return;
    }
    if (result === HUMAN_RESULT.INVALID) {
        setStatus('その手は打てません。別のマスを選んでください。');
        return;
    }
    if (result === HUMAN_RESULT.FORCED_PASS) {
        setStatus('合法手がないため自動的にパスしました。');
    } else if (result === HUMAN_RESULT.OK) {
        setStatus(`あなたが ${toCoordinate(row, col)} に打ちました。`);
    }
    renderBoard();
    updateScores();
    if (state.api.webIsGameOver() === 1) {
        announceWinner();
        return;
    }
    queueAiMove();
}

function handlePassClick() {
    if (!state.api || state.boardLocked) {
        return;
    }
    const result = state.api.webHumanPass();
    if (result === HUMAN_RESULT.INVALID) {
        setStatus('合法手があるためパスできません。');
        return;
    }
    if (result === HUMAN_RESULT.NOT_TURN) {
        setStatus('今はあなたの番ではありません。');
        return;
    }
    if (result === HUMAN_RESULT.GAME_OVER) {
        announceWinner();
        return;
    }
    setStatus('パスしました。');
    renderBoard();
    updateScores();
    if (state.api.webIsGameOver() === 1) {
        announceWinner();
        return;
    }
    queueAiMove();
}

function startGame() {
    state.humanColor = Number(elements.colorSelect.value);
    state.api.webReset(state.humanColor);
    state.humanColor = state.api.webGetHumanPlayer();
    state.boardLocked = state.humanColor === 1; // 人間が白ならAIが先手
    renderBoard();
    updateScores();
    if (state.humanColor === 1) {
        setStatus('AIが初手を考えています...');
        queueAiMove();
    } else {
        state.boardLocked = false;
        renderBoard();
        setStatus('あなたが先手です。好きな場所に打ちましょう。');
    }
}

function initUi() {
    buildBoard();
    elements.newGame.addEventListener('click', () => {
        startGame();
    });
    elements.pass.addEventListener('click', () => handlePassClick());
}

function initialize() {
    initUi();
    // set initial board size and keep it updated on resize/orientation changes
    adjustBoardSize();
    window.addEventListener('resize', adjustBoardSize);
    window.addEventListener('orientationchange', adjustBoardSize);
    if (typeof createReversiModule !== 'function') {
        setStatus('WASM モジュールが読み込めませんでした。先に make wasm を実行してください。');
        return;
    }
    createReversiModule().then((Module) => {
        state.module = Module;
        state.api = bindApi(Module);
        setStatus('準備完了。');
        startGame();
    }).catch((error) => {
        console.error('Failed to load WASM module:', error);
        setStatus('WASM モジュールの読み込みに失敗しました。コンソールを確認してください。');
    });
}

initialize();
