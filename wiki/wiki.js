const $ = (seletor) => document.querySelector(seletor);

async function carregarWiki() {
  const resposta = await fetch('lore.json');
  if (!resposta.ok) throw new Error('O arquivo da história não pôde ser aberto.');
  const dados = await resposta.json();
  $('#hero-fundo').style.backgroundImage = `url('${dados.capa}')`;
  $('#imagem-grupo').src = dados.grupo;
  $('#imagem-final').src = dados.final;

  const pessoas = $('#grade-personagens');
  for (const pessoa of dados.personagens) {
    const card = document.createElement('article'); card.className = 'personagem';
    const img = document.createElement('img'); img.src = pessoa.imagem; img.alt = `Retrato de ${pessoa.nome}`; img.loading = 'lazy';
    const texto = document.createElement('div');
    const h3 = document.createElement('h3'); h3.textContent = pessoa.nome;
    const p = document.createElement('p'); p.textContent = pessoa.descricao;
    texto.append(h3, p); card.append(img, texto); pessoas.append(card);
  }
  const locais = $('#grade-locais');
  for (const [i, local] of dados.locais.entries()) {
    const card = document.createElement('article'); card.className = 'local';
    const img = document.createElement('img'); img.src = local.imagem; img.alt = local.nome; img.loading = 'lazy';
    const texto = document.createElement('div');
    const indice = document.createElement('span'); indice.className = 'local-indice'; indice.textContent = String(i).padStart(2, '0');
    const h3 = document.createElement('h3'); h3.textContent = local.nome;
    const p = document.createElement('p'); p.textContent = local.descricao;
    texto.append(indice, h3, p); card.append(img, texto); locais.append(card);
  }
  const capitulos = $('#lista-capitulos');
  for (const [i, capitulo] of dados.capitulos.entries()) {
    const secao = document.createElement('section'); secao.className = 'capitulo'; secao.id = capitulo.id;
    const cab = document.createElement('div'); cab.className = 'capitulo-cab';
    const numero = document.createElement('span'); numero.textContent = String(i + 1).padStart(2, '0');
    const h3 = document.createElement('h3'); h3.textContent = capitulo.titulo;
    cab.append(numero, h3); secao.append(cab);
    for (const [j, cena] of capitulo.cenas.entries()) {
      const figura = document.createElement('figure'); figura.className = 'cena';
      if (cena.imagem) {
        const img = document.createElement('img'); img.src = cena.imagem;
        img.alt = `Ilustração da cena ${j + 1} de ${capitulo.titulo}`; img.loading = 'lazy'; figura.append(img);
      }
      const legenda = document.createElement('figcaption'); legenda.textContent = cena.texto;
      figura.append(legenda); secao.append(figura);
    }
    capitulos.append(secao);
  }
  $('#revelar').addEventListener('click', () => {
    capitulos.hidden = false;
    $('.aviso-spoiler').hidden = true;
    capitulos.scrollIntoView({behavior: 'smooth'});
  });
}
carregarWiki().catch(erro => { $('#lista-capitulos').hidden = false; $('#lista-capitulos').textContent = erro.message; });
