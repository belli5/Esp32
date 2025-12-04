import React, { useEffect, useMemo, useState, useRef } from "react";
import { useNavigate, useLocation } from "react-router-dom";
import mqtt from "mqtt";

import {
  PageWrapper,
  Card,
  Title,
  Subtitle,
  TabsBar,
  TabsTrack,
  Slider,
  TabButton,
  ChartsGrid,
  ChartCard,
  ChartTitle,
  ChartSubtitle,
  ChartArea,
  BarRow,
  Bar,
  BarLabelRow,
  BarLabel,
  WeekSummaryCard,
  WeekSummaryTitle,
  WeekDaysRow,
  DayPill,
  LegendRow,
  LegendItem,
  LegendDot,
  LogList,
  LogRow,
  LogMain,
  LogMeta,
  LogStrong,
  EmptyLog,
} from "./dashboard.styles";

const MQTT_URL = "ws://172.20.10.2:9001";

const diasOrdem = ["Seg", "Ter", "Qua", "Qui", "Sex"];
const mapDowToLabel = {
  1: "Seg",
  2: "Ter",
  3: "Qua",
  4: "Qui",
  5: "Sex",
};

function parseDateBR(dateStr) {
  const [d, m, y] = dateStr.split("/").map(Number);
  return new Date(y, m - 1, d);
}

// Mapeia o nome completo que o ESP envia ("Segunda-feira") para rótulo curto ("Seg")
const mapNomeCompletoToShort = {
  "Domingo": "Dom",
  "Segunda-feira": "Seg",
  "Terca-feira": "Ter",
  "Quarta-feira": "Qua",
  "Quinta-feira": "Qui",
  "Sexta-feira": "Sex",
  "Sabado": "Sab",
};

export default function Dashboard() {
  const navigate = useNavigate();
  const location = useLocation();
  const [insideList, setInsideList] = useState([]);

  const [movs, setMovs] = useState([]);              // histórico de movimentações
  const [uidOptions, setUidOptions] = useState([]);  // UIDs disponíveis para o select
  const [selectedUid, setSelectedUid] = useState(""); // UID selecionado
  const [uidWeekDays, setUidWeekDays] = useState([]); // dias recebidos do ESP para o UID selecionado

  const clientRef = useRef(null);

  let activeIndex = 0;
  if (location.pathname === "/cadastro") activeIndex = 1;
  else if (location.pathname === "/dashboard") activeIndex = 2;

  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

  // Conecta no MQTT e trata mensagens
  useEffect(() => {
    const client = mqtt.connect(MQTT_URL);
    clientRef.current = client;

    client.on("connect", () => {
      console.log("MQTT conectado no FRONT!");
      client.subscribe("portaria/movimentacoes");
      client.subscribe("portaria/dentro");
      client.subscribe("portaria/status"); // 👈 para receber uid_week_days

      // pedir histórico de movimentações
      client.publish(
        "portaria/comandos",
        JSON.stringify({ cmd: "get_history" })
      );

      // pedir lista de quem está dentro hoje (equivalente ao 'p')
      client.publish(
        "portaria/comandos",
        JSON.stringify({ cmd: "get_inside_today" })
      );
    });

    client.on("message", (topic, msg) => {
      try {
        const data = JSON.parse(msg.toString());
        console.log("MQTT msg:", topic, data);

        if (topic === "portaria/movimentacoes") {
          setMovs((prev) => [...prev, data]);
        } else if (topic === "portaria/dentro") {
          // payload: { context: "inside", total: X, itens: [ { uid, count }, ... ] }
          setInsideList(data.itens || []);
        } else if (topic === "portaria/status") {
          // Aqui podem vir vários contexts diferentes. Queremos o "uid_week_days".
          if (data.context === "uid_week_days") {
            // data: { context, uid, totalDias, dias: [ "Segunda-feira", ... ] }
            setUidWeekDays(data.dias || []);
          }
        }
      } catch (e) {
        console.error("MQTT JSON inválido:", e);
      }
    });

    return () => {
      if (clientRef.current) {
        clientRef.current.end(true);
        clientRef.current = null;
      }
    };
  }, []);

  // Monta a lista de UIDs possíveis a partir das movimentações (funcionário + usuário)
  useEffect(() => {
    const set = new Set();
    movs.forEach((m) => {
      if (m.funcionario) set.add(m.funcionario);
      if (m.usuario) set.add(m.usuario);
    });
    const arr = Array.from(set);
    setUidOptions(arr);

    // se ainda não houver um UID selecionado, seleciona o primeiro automaticamente
    if (!selectedUid && arr.length > 0) {
      setSelectedUid(arr[0]);
    }
  }, [movs, selectedUid]);

  // Sempre que o UID selecionado mudar, pede pro ESP os dias de semana desse UID
  useEffect(() => {
    if (!selectedUid) return;
    const client = clientRef.current;
    if (!client || !client.connected) return;

    const payload = {
      cmd: "get_uid_week_days",
      uid: selectedUid,
    };

    console.log("Publicando get_uid_week_days:", payload);

    client.publish("portaria/comandos", JSON.stringify(payload));
  }, [selectedUid]);

  // 📊 Transformar movimentações em dados pros gráficos (geral, não por UID)
  const {
    presencasSemana,
    semanaResumoAll,
    presencasAbsolutas,
  } = useMemo(() => {
    const now = new Date();
    const mesAtual = now.getMonth();
    const anoAtual = now.getFullYear();

    const contDiaSem = { Seg: 0, Ter: 0, Qua: 0, Qui: 0, Sex: 0 };

    // --- presença no mês ---
    movs.forEach(({ data }) => {
      const d = parseDateBR(data);

      if (d.getMonth() !== mesAtual || d.getFullYear() !== anoAtual) return;

      const label = mapDowToLabel[d.getDay()];
      if (label && contDiaSem[label] !== undefined) {
        contDiaSem[label] += 1;
      }
    });

    const maxDia = Math.max(1, ...diasOrdem.map((d) => contDiaSem[d]));
    const pres = diasOrdem.map((dia) =>
      Math.round((contDiaSem[dia] / maxDia) * 100)
    );

    return {
      presencasSemana: pres,
      semanaResumoAll: diasOrdem.map((dia) => ({
        dia,
        presente: contDiaSem[dia] > 0,
      })),
      presencasAbsolutas: contDiaSem,
    };
  }, [movs]);

  // Resumo da semana para o UID selecionado, baseado na resposta do ESP (uidWeekDays)
  const semanaResumoUid = useMemo(() => {
    // uidWeekDays vem algo tipo ["Segunda-feira", "Quarta-feira", ...]
    const presentesSet = new Set(
      uidWeekDays.map((nome) => mapNomeCompletoToShort[nome] || nome)
    );

    return diasOrdem.map((dia) => ({
      dia,
      presente: presentesSet.has(dia),
    }));
  }, [uidWeekDays]);

  // últimos 10 registros (mais recentes em cima)
  const movsRecentes = useMemo(() => {
    return [...movs].slice(-10).reverse();
  }, [movs]);

  return (
    <PageWrapper>
      <Card>
        <TabsBar>
          <TabsTrack>
            <Slider activeIndex={activeIndex} />
            <TabButton active={activeIndex === 0} onClick={goHome}>
              Home
            </TabButton>
            <TabButton active={activeIndex === 1} onClick={goCadastro}>
              Cadastro
            </TabButton>
            <TabButton active={activeIndex === 2} onClick={goDashboard}>
              Dashboard
            </TabButton>
          </TabsTrack>
        </TabsBar>

        <Title>Dashboard</Title>
        <Subtitle>Dados reais recebidos da portaria via MQTT</Subtitle>

        <ChartsGrid>
          {/* Gráfico 1 */}
          <ChartCard>
            <ChartTitle>Presença no Mês</ChartTitle>
            <ChartSubtitle>Com base nas movimentações</ChartSubtitle>

            <ChartArea>
              <BarRow>
                {presencasSemana.map((v, i) => {
                  const dia = diasOrdem[i];
                  const valorReal = presencasAbsolutas[dia] || 0;

                  // se quiser esconder barras de 0, você pode trocar value={v} por value={valorReal > 0 ? v : 0}
                  return (
                    <Bar
                      key={dia}
                      value={v} // percent
                      data-count={valorReal > 0 ? valorReal : ""}
                      title={
                        valorReal > 0
                          ? `${dia}: ${valorReal} movimentações`
                          : ""
                      }
                    />
                  );
                })}
              </BarRow>

              <BarLabelRow>
                {diasOrdem.map((d) => (
                  <BarLabel key={d}>{d}</BarLabel>
                ))}
              </BarLabelRow>
            </ChartArea>
          </ChartCard>

          {/* Log de quem está dentro hoje */}
          <ChartCard>
            <ChartTitle>Quem está dentro da escola</ChartTitle>
            <ChartSubtitle>Baseado nas movimentações de hoje</ChartSubtitle>

            <ChartArea>
              <LogList>
                {insideList.length === 0 ? (
                  <EmptyLog>Ninguém dentro no momento.</EmptyLog>
                ) : (
                  insideList.map((item) => (
                    <LogRow key={item.uid}>
                      <LogMain>
                        <LogStrong>{item.uid}</LogStrong>
                      </LogMain>

                      <LogMeta>
                        {item.count > 1 ? (
                          <span>{item.count} entradas não fechadas</span>
                        ) : (
                          <span>1 entrada não fechada</span>
                        )}
                      </LogMeta>
                    </LogRow>
                  ))
                )}
              </LogList>
            </ChartArea>
          </ChartCard>

          {/* Log de movimentações */}
          <ChartCard>
            <ChartTitle>Log de movimentações</ChartTitle>
            <ChartSubtitle>Quem recebeu/liberou e quando</ChartSubtitle>

            <ChartArea>
              <LogList>
                {movsRecentes.length === 0 ? (
                  <EmptyLog>Nenhuma movimentação recebida ainda.</EmptyLog>
                ) : (
                  movsRecentes.map((m, idx) => (
                    <LogRow key={idx}>
                      <LogMain>
                        <LogStrong>{m.funcionario}</LogStrong>
                        <span> → </span>
                        <LogStrong>{m.usuario}</LogStrong>
                      </LogMain>

                      <LogMeta>
                        <span>{m.data}</span>
                        <span> • </span>
                        <span>{m.hora}</span>
                      </LogMeta>
                    </LogRow>
                  ))
                )}
              </LogList>
            </ChartArea>
          </ChartCard>
        </ChartsGrid>

        {/* Resumo Semana por ID */}
        <WeekSummaryCard>
          <WeekSummaryTitle>
            Resumo da Semana
          </WeekSummaryTitle>

          {/* Select de ID */}
          <div style={{ marginBottom: "16px", display: "flex", gap: 8, alignItems: "center" }}>
            <span>Selecionar ID:</span>
            <select
              value={selectedUid}
              onChange={(e) => setSelectedUid(e.target.value)}
            >
              <option value="">-- escolha um ID --</option>
              {uidOptions.map((uid) => (
                <option key={uid} value={uid}>
                  {uid}
                </option>
              ))}
            </select>
          </div>

          <WeekDaysRow>
            {semanaResumoUid.map((d) => (
              <DayPill key={d.dia} presente={d.presente}>
                {d.dia}
              </DayPill>
            ))}
          </WeekDaysRow>

          <LegendRow>
            <LegendItem>
              <LegendDot type="presente" /> Foi
            </LegendItem>
            <LegendItem>
              <LegendDot type="ausente" /> Não foi
            </LegendItem>
          </LegendRow>
        </WeekSummaryCard>
      </Card>
    </PageWrapper>
  );
}
