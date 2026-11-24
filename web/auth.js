/* Entrada e cartão de memória exclusivos da versão web na AWS. */
(() => {
  const nomes = ['perfil.txt', 'recordes.txt', 'telemetria.csv',
    ...Array.from({length: 15}, (_, i) => `bloco_${String(i + 1).padStart(2, '0')}.txt`)];
  const paises = `AD AE AF AG AI AL AM AO AQ AR AS AT AU AW AX AZ BA BB BD BE BF BG BH BI BJ BL BM BN BO BQ BR BS BT BV BW BY BZ CA CC CD CF CG CH CI CK CL CM CN CO CR CU CV CW CX CY CZ DE DJ DK DM DO DZ EC EE EG EH ER ES ET FI FJ FK FM FO FR GA GB GD GE GF GG GH GI GL GM GN GP GQ GR GS GT GU GW GY HK HM HN HR HT HU ID IE IL IM IN IO IQ IR IS IT JE JM JO JP KE KG KH KI KM KN KP KR KW KY KZ LA LB LC LI LK LR LS LT LU LV LY MA MC MD ME MF MG MH MK ML MM MN MO MP MQ MR MS MT MU MV MW MX MY MZ NA NC NE NF NG NI NL NO NP NR NU NZ OM PA PE PF PG PH PK PL PM PN PR PS PT PW PY QA RE RO RS RU RW SA SB SC SD SE SG SH SI SJ SK SL SM SN SO SR SS ST SV SX SY SZ TC TD TF TG TH TJ TK TL TM TN TO TR TT TV TW TZ UA UG UM US UY UZ VA VC VE VG VI VN VU WF WS YE YT ZA ZM ZW`.split(' ');
  const acesso = document.getElementById('acesso');
  const aviso = document.getElementById('acesso-estado');
  const botoes = document.getElementById('acesso-botoes');
  const paisArea = document.getElementById('pais-area');
  const paisSelect = document.getElementById('pais-select');
  const ranking = document.getElementById('ranking');
  const feedback = document.getElementById('feedback');
  const loreFallback = document.getElementById('lore-fallback');
  const conflitoPainel = document.getElementById('conflito');
  const status = document.getElementById('save-status');
  let config = null, tokens = null, fs = null, version = null, ultimo = null;
  let modo = null, ocupada = false, concluido = null;
  let jogandoAntes = false;
  let rankingDados = null, rankingDificuldade = 'normal';
  let administrador = false;
  let conflito = false;
  const demos = {
    galeria: '?bioma=1',
    mike: '?sala=b3_c02&forte=1&amigo=mike',
    stan: '?sala=b1_c03&amigo=stan',
    pennywise2: '?sala=b3_chefe&forte=1&pennywise=1',
    pennywise3: '?sala=b5_chefe&forte=1&pennywise=1',
    covil: '?sala=b7_covil&forte=1',
    epilogo: '?cena=final'
  };
  function parametrosDemo() {
    if (!config) return location.search || ''; // só desenvolvimento local, sem AWS
    const slug = new URLSearchParams(location.search).get('demo');
    return administrador && Object.hasOwn(demos, slug) ? demos[slug] : '';
  }

  const contaPainel = document.getElementById('conta');
  const contaBotao = document.getElementById('conta-botao');
  // Painéis por cima do jogo. Podem empilhar (ex.: Conta e Conflito); o jogo só volta quando todos fecham.
  const paineis = () => [ranking, feedback, loreFallback, document.getElementById('demos'), conflitoPainel, contaPainel];
  const algumAberto = () => paineis().some(p => !p.hidden);
  function sobrepor(painel) {
    if (!algumAberto()) jogandoAntes = !!window.ecosJogando;
    window.ecosJogando = false;
    painel.hidden = false;
  }
  function fechar(painel) {
    painel.hidden = true;
    if (algumAberto()) return;
    window.ecosJogando = jogandoAntes;
    document.getElementById('tela').focus();
  }

  const mostrar = (texto) => { aviso.textContent = texto; };
  const bandeira = (codigo) => /^[A-Z]{2}$/.test(codigo || '')
    ? [...codigo].map(c => String.fromCodePoint(127397 + c.charCodeAt(0))).join('') : '🌐';
  function arquivoAtual() {
    const files = {};
    for (const nome of nomes) {
      try { files[nome] = fs.readFile(`/saves/${nome}`, {encoding: 'utf8'}); } catch (_) {}
    }
    return files;
  }
  function aplicarArquivos(files) {
    for (const [nome, texto] of Object.entries(files || {})) {
      if (!nomes.includes(nome) || typeof texto !== 'string') throw Error('Cartão recebido inválido.');
      fs.writeFile(`/saves/${nome}`, texto);
    }
  }
  function tokenAtual() {
    const guardado = sessionStorage.getItem('ecosTokens');
    try { return guardado ? JSON.parse(guardado) : null; } catch (_) { return null; }
  }
  async function renovar() {
    if (!tokens) throw Error('Entre na sua conta novamente.');
    if (Date.now() < tokens.expires - 60000) return;
    if (!tokens.refresh_token) throw Error('Sessão encerrada. Entre novamente.');
    const body = new URLSearchParams({grant_type: 'refresh_token', client_id: config.clientId,
      refresh_token: tokens.refresh_token});
    const res = await fetch(`${config.cognitoDomain}/oauth2/token`, {method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body});
    if (!res.ok) throw Error('Sessão encerrada. Entre novamente.');
    const dados = await res.json();
    tokens = {...tokens, ...dados, expires: Date.now() + dados.expires_in * 1000};
    sessionStorage.setItem('ecosTokens', JSON.stringify(tokens));
  }
  async function api(path, method = 'GET', body = null, privado = true) {
    const headers = {};
    if (privado) { await renovar(); headers.Authorization = `Bearer ${tokens.access_token}`; }
    if (body) headers['Content-Type'] = 'application/json';
    const res = await fetch(`${config.apiUrl}${path}`, {method, headers,
      body: body ? JSON.stringify(body) : undefined, cache: 'no-store'});
    const dados = await res.json().catch(() => ({}));
    if (!res.ok) {
      const erro = Error(dados.erro || `Serviço indisponível (${res.status}).`);
      erro.status = res.status;
      throw erro;
    }
    return dados;
  }
  async function desafio() {
    const bytes = crypto.getRandomValues(new Uint8Array(32));
    const verifier = [...bytes].map(x => x.toString(16).padStart(2, '0')).join('');
    const hash = new Uint8Array(await crypto.subtle.digest('SHA-256', new TextEncoder().encode(verifier)));
    const challenge = btoa(String.fromCharCode(...hash)).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
    const state = crypto.randomUUID();
    sessionStorage.setItem('ecosOauth', JSON.stringify({state, verifier}));
    return {challenge, state};
  }
  async function autenticar(tela) {
    const {challenge, state} = await desafio();
    const path = tela === 'entrar' ? '/oauth2/authorize' : tela === 'criar' ? '/signup' : '/forgotPassword';
    const query = new URLSearchParams({client_id: config.clientId, response_type: 'code',
      scope: 'openid email', redirect_uri: config.redirectUri, code_challenge_method: 'S256',
      code_challenge: challenge, state});
    location.assign(`${config.cognitoDomain}${path}?${query}`);
  }
  async function retornoOAuth() {
    const params = new URLSearchParams(location.search);
    if (params.has('error')) throw Error('Não foi possível entrar: ' + (params.get('error_description') || params.get('error')));
    const code = params.get('code');
    if (!code) return false;
    const pendente = JSON.parse(sessionStorage.getItem('ecosOauth') || '{}');
    sessionStorage.removeItem('ecosOauth');
    history.replaceState(null, '', location.pathname);
    if (!pendente.state || pendente.state !== params.get('state')) throw Error('Resposta de login inválida.');
    const body = new URLSearchParams({grant_type: 'authorization_code', client_id: config.clientId,
      code, redirect_uri: config.redirectUri, code_verifier: pendente.verifier});
    const res = await fetch(`${config.cognitoDomain}/oauth2/token`, {method: 'POST',
      headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body});
    if (!res.ok) throw Error('Não foi possível concluir o login.');
    tokens = await res.json();
    tokens.expires = Date.now() + tokens.expires_in * 1000;
    sessionStorage.setItem('ecosTokens', JSON.stringify(tokens));
    return true;
  }
  function escolherPais() {
    paisSelect.replaceChildren();
    const nomesRegiao = typeof Intl.DisplayNames === 'function'
      ? new Intl.DisplayNames(['pt-BR'], {type: 'region'}) : null;
    const opcoes = paises.map(codigo => ({codigo, nome: nomesRegiao ? nomesRegiao.of(codigo) : codigo}));
    opcoes.sort((a, b) => a.nome.localeCompare(b.nome, 'pt-BR'));
    for (const {codigo, nome} of opcoes) {
      const opt = document.createElement('option');
      opt.value = codigo; opt.textContent = `${bandeira(codigo)} ${nome}`;
      paisSelect.append(opt);
    }
    const sugerido = (navigator.language || '').split('-')[1]?.toUpperCase();
    paisSelect.value = paises.includes(sugerido) ? sugerido : 'BR';
    paisArea.hidden = false;
  }
  function pronto(escolha) {
    modo = escolha;
    acesso.hidden = true;
    contaBotao.hidden = escolha !== 'conta';
    concluido();
  }
  async function carregarConta() {
    mostrar('Abrindo seu cartão de memória na AWS...');
    const [cartao, perfil] = await Promise.all([api('/saves'), api('/profile')]);
    administrador = await api('/admin').then(d => d.allowed === true).catch(() => false);
    document.getElementById('admin-demos').hidden = !administrador;
    aplicarArquivos(cartao.files);
    version = cartao.version;
    ultimo = JSON.stringify(arquivoAtual());
    if (!perfil.country) {
      mostrar('Escolha o país que aparecerá ao lado do seu apelido no ranking.');
      botoes.hidden = true;
      escolherPais();
      return;
    }
    status.dataset.conta = `${bandeira(perfil.country)} ${perfil.nick || 'Conta'}`;
    status.textContent = `${status.dataset.conta} · cartão na AWS`;
    pronto('conta');
  }
  async function preparar(arquivos) {
    fs = arquivos;
    acesso.hidden = false;
    const fim = new Promise(resolve => { concluido = resolve; });
    try {
      const res = await fetch('config.json', {cache: 'no-store'});
      if (res.ok) config = await res.json();
      if (config && config.apiUrl && config.cognitoDomain && config.clientId && config.redirectUri) {
        botoes.hidden = false;
        tokens = tokenAtual();
        const retornou = await retornoOAuth();
        if (retornou || tokens) await carregarConta();
        else mostrar('Entre na sua conta ou jogue sem conta.');
      } else {
        document.querySelectorAll('[data-entrar]').forEach(b => { b.hidden = true; });
        mostrar('Versão local de demonstração. O progresso termina ao fechar ou recarregar a página.');
      }
    } catch (erro) { mostrar(erro.message + ' Você pode jogar sem conta.'); botoes.hidden = false; }
    return fim;
  }
  async function sincronizar() {
    if (modo !== 'conta' || ocupada || conflito) return;
    const atual = JSON.stringify(arquivoAtual());
    if (atual === ultimo) return;
    ocupada = true;
    status.textContent = 'Salvando cartão na AWS...';
    try {
      const dados = await api('/saves', 'PUT', {files: JSON.parse(atual), version});
      version = dados.version;
      ultimo = atual;
      status.textContent = dados.rankingPending ? 'Cartão salvo. Ranking aguardando atualização.' : 'Cartão salvo na AWS';
    } catch (erro) {
      status.textContent = erro.message;
      if (erro.status === 409) { conflito = true; sobrepor(conflitoPainel); }
    }
    finally { ocupada = false; }
  }
  async function abrirRanking() {
    sobrepor(ranking);
    ranking.querySelector('[data-lista]').textContent = 'Carregando...';
    if (modo !== 'conta') {
      ranking.querySelector('[data-lista]').textContent = 'Entre na sua conta para ver o ranking mundial.';
      ranking.querySelector('[data-pessoal]').textContent = 'O progresso sem conta termina ao fechar ou recarregar a página.';
      return;
    }
    try {
      const dados = await api('/ranking');
      rankingDados = dados.ranking;
      mostrarRanking();
      const pessoal = ranking.querySelector('[data-pessoal]');
      pessoal.replaceChildren();
      if (modo === 'conta') {
        const perfil = await api('/profile');
        perfil.top5.forEach((registro, i) => {
          const linha = document.createElement('div'); linha.className = 'linha-ranking';
          const nome = document.createElement('span'); nome.textContent = `${i + 1}. ${Number(registro.score).toLocaleString('pt-BR')} pontos`;
          const dif = document.createElement('span');
          dif.textContent = {facil: 'Fácil', normal: 'Normal', dificil: 'Difícil'}[registro.difficulty] || '';
          linha.append(nome, dif); pessoal.append(linha);
        });
        if (!perfil.top5.length) pessoal.textContent = 'Você ainda não tem pontuações.';
      } else pessoal.textContent = 'Entre na sua conta para ter um histórico pessoal.';
    } catch (erro) { ranking.querySelector('[data-lista]').textContent = erro.message; }
  }
  function mostrarRanking() {
      const entradas = rankingDados?.[rankingDificuldade] || [];
      const lista = ranking.querySelector('[data-lista]');
      lista.replaceChildren();
      if (!entradas.length) lista.textContent = 'Ainda não há pontuações nesta dificuldade.';
      entradas.forEach((r, i) => {
        const linha = document.createElement('div');
        linha.className = 'linha-ranking';
        const nome = document.createElement('span');
        nome.textContent = `${i + 1}. ${bandeira(r.country)} ${r.nick}`;
        const pontos = document.createElement('strong');
        pontos.textContent = Number(r.score).toLocaleString('pt-BR');
        linha.append(nome, pontos); lista.append(linha);
      });
      ranking.querySelectorAll('[data-dificuldade]').forEach(b =>
        b.setAttribute('aria-selected', String(b.dataset.dificuldade === rankingDificuldade)));
  }
  function abrirLore() {
    const url = config?.wikiUrl || 'https://diegof-d2fs.github.io/it-ecos-de-derry/';
    const novaAba = window.open(url, '_blank');
    if (novaAba) { novaAba.opener = null; return; }
    document.getElementById('lore-link').href = url;
    document.getElementById('lore-frame').src = url;
    sobrepor(loreFallback);
  }
  function abrirFeedback() {
    // Sem alert(): um diálogo do navegador congela o jogo e o controle.
    const semConta = modo !== 'conta';
    document.getElementById('feedback-form').querySelectorAll('input, textarea, select, button[type="submit"]')
      .forEach(campo => { campo.disabled = semConta; });
    document.getElementById('feedback-estado').textContent = semConta
      ? 'Entre na sua conta para enviar feedback. Sem conta, o formulário fica bloqueado.' : '';
    sobrepor(feedback);
    document.getElementById(semConta ? 'feedback-fechar' : 'feedback-tipo').focus();
  }
  function abrirConta() {
    if (modo !== 'conta') return;
    document.getElementById('conta-quem').textContent = status.dataset.conta || '';
    document.getElementById('conta-estado').textContent = 'Seu cartão fica guardado na AWS. Ao sair, esta aba volta à tela de entrada.';
    document.getElementById('conta-forcar').hidden = true;
    sobrepor(contaPainel);
    document.getElementById('conta-voltar').focus();
  }
  // Sair da conta: salva o que falta, apaga os tokens desta aba e encerra a sessão do Cognito
  // (sem isso, o próximo "Entrar" num computador compartilhado entraria direto na mesma conta).
  async function sairDaConta(mesmoSemSalvar) {
    const estado = document.getElementById('conta-estado');
    estado.textContent = 'Salvando o cartão antes de sair...';
    while (ocupada) await new Promise(r => setTimeout(r, 100));
    if (!mesmoSemSalvar) {
      await sincronizar();
      if (conflito || JSON.stringify(arquivoAtual()) !== ultimo) {
        estado.textContent = 'O cartão não foi salvo. Se sair agora, o progresso desde o último salvamento será perdido.';
        document.getElementById('conta-forcar').hidden = false;
        return;
      }
    }
    modo = null; tokens = null;
    sessionStorage.removeItem('ecosTokens');
    sessionStorage.removeItem('ecosOauth');
    window.ecosJogando = false;
    location.assign(`${config.cognitoDomain}/logout?` +
      new URLSearchParams({client_id: config.clientId, logout_uri: config.redirectUri}));
  }
  document.getElementById('feedback-form').addEventListener('submit', async e => {
    e.preventDefault();
    const estado = document.getElementById('feedback-estado');
    estado.textContent = 'Enviando...';
    const body = {
      type: document.getElementById('feedback-tipo').value,
      title: document.getElementById('feedback-titulo').value.trim(),
      message: document.getElementById('feedback-mensagem').value.trim(),
      email: document.getElementById('feedback-email').value.trim(),
      apelido: document.getElementById('feedback-apelido').value
    };
    try {
      const resultado = await api('/feedback', 'POST', body);
      estado.textContent = resultado.emailed
        ? 'Obrigado! Seu feedback foi recebido e enviado por e-mail.'
        : 'Feedback guardado. O aviso por e-mail está pendente.';
      e.target.reset();
    } catch (erro) { estado.textContent = erro.message; }
  });
  document.querySelectorAll('[data-entrar]').forEach(b => b.addEventListener('click', e => {
    e.stopPropagation(); autenticar(b.dataset.entrar).catch(erro => mostrar(erro.message));
  }));
  document.getElementById('sem-conta').addEventListener('click', e => {
    e.stopPropagation(); status.textContent = 'Sem conta: progresso apenas nesta sessão.'; pronto('convidado');
  });
  document.getElementById('pais-confirmar').addEventListener('click', async e => {
    e.stopPropagation();
    try {
      const dados = await api('/profile', 'PUT', {country: paisSelect.value});
      status.dataset.conta = `${bandeira(dados.country)} ${dados.nick}`;
      status.textContent = `${status.dataset.conta} · cartão na AWS`;
      pronto('conta');
    } catch (erro) { mostrar(erro.message); }
  });
  document.getElementById('ranking-fechar').addEventListener('click', () => fechar(ranking));
  ranking.querySelectorAll('[data-dificuldade]').forEach(b => b.addEventListener('click', () => {
    rankingDificuldade = b.dataset.dificuldade; mostrarRanking();
  }));
  document.getElementById('feedback-fechar').addEventListener('click', () => fechar(feedback));
  document.getElementById('lore-fechar').addEventListener('click', () => {
    document.getElementById('lore-frame').src = 'about:blank'; fechar(loreFallback);
  });
  document.getElementById('admin-demos').addEventListener('click', () =>
    sobrepor(document.getElementById('demos')));
  document.getElementById('demos-fechar').addEventListener('click', () =>
    fechar(document.getElementById('demos')));
  document.querySelectorAll('[data-demo]').forEach(b => b.addEventListener('click', () => {
    if (!administrador) return;
    location.assign(`${location.pathname}?demo=${encodeURIComponent(b.dataset.demo)}`);
  }));
  document.getElementById('conflito-recarregar').addEventListener('click', () => location.reload());
  document.getElementById('conflito-sobrescrever').addEventListener('click', async () => {
    const estado = document.getElementById('conflito-estado');
    estado.textContent = 'Salvando a versão desta aba...';
    try {
      const atual = JSON.stringify(arquivoAtual());
      const remoto = await api('/saves');
      const salvo = await api('/saves', 'PUT', {files: JSON.parse(atual), version: remoto.version});
      version = salvo.version; ultimo = atual; conflito = false;
      status.textContent = 'Cartão salvo na AWS'; fechar(conflitoPainel);
    } catch (erro) { estado.textContent = erro.message; }
  });
  contaBotao.addEventListener('click', abrirConta);
  document.getElementById('conta-voltar').addEventListener('click', () => fechar(contaPainel));
  document.getElementById('conta-sair').addEventListener('click', () => sairDaConta(false));
  document.getElementById('conta-forcar').addEventListener('click', () => sairDaConta(true));
  // Fechar ou trocar de aba: tenta salvar já, sem esperar o ciclo de 3 s do jogo.
  document.addEventListener('visibilitychange', () => { if (document.hidden) sincronizar(); });
  // Esc e o botão B fecham os painéis informativos; o de conflito exige uma escolha.
  const fechaveis = () => [contaPainel, ranking, feedback, loreFallback, document.getElementById('demos')];
  window.addEventListener('keydown', e => {
    if (e.key === 'Escape') {
      for (const painel of fechaveis()) if (!painel.hidden) {
        e.stopImmediatePropagation(); fechar(painel); break;
      }
    }
  }, true);
  let bApertado = false;
  setInterval(() => {
    const aberto = fechaveis().find(p => !p.hidden);
    if (!aberto) { bApertado = false; return; }
    const apertado = [...(navigator.getGamepads?.() || [])].some(g => g?.buttons?.[1]?.pressed);
    if (apertado && !bApertado) fechar(aberto);
    bApertado = apertado;
  }, 100);
  window.ecosAcesso = {preparar, sincronizar, abrirRanking, abrirLore, abrirFeedback, parametrosDemo};
})();
