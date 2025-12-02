import React, { useEffect, useMemo, useState } from "react";
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

const MQTT_URL = "ws://172.20.10.4:9001";

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

export default function Dashboard() {
  const navigate = useNavigate();
  const location = useLocation();

  let activeIndex = 0;
  if (location.pathname === "/cadastro") activeIndex = 1;
  else if (location.pathname === "/dashboard") activeIndex = 2;

  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

  // 🟦 Array de movimentações recebidas via MQTT
  const [movs, setMovs] = useState([]);

  useEffect(() => {
    const client = mqtt.connect(MQTT_URL);

    client.on("connect", () => {
      console.log("MQTT conectado no FRONT!");
      client.subscribe("portaria/movimentacoes");

      const cmd = {
        cmd: "get_history",
      };
      client.publish("portaria/comandos", JSON.stringify(cmd));
    });

    client.on("message", (topic, msg) => {
      if (topic !== "portaria/movimentacoes") return;

      try {
        const data = JSON.parse(msg.toString());
        console.log("Mov recebida:", data);
        setMovs((prev) => [...prev, data]);
      } catch (e) {
        console.error("MQTT JSON inválido:", e);
      }
    });

    return () => client.end(true);
  }, []);

  // 📊 Transformar movimentações em dados pros gráficos
  const {
    presencasSemana,
    semanaResumo,
    entradasVsSaidas,
    presencasAbsolutas,
  } = useMemo(() => {
    const now = new Date();
    const mesAtual = now.getMonth();
    const anoAtual = now.getFullYear();

    const contDiaSem = { Seg: 0, Ter: 0, Qua: 0, Qui: 0, Sex: 0 };

    movs.forEach(({ data }) => {
      const d = parseDateBR(data);

      if (d.getMonth() !== mesAtual || d.getFullYear() !== anoAtual) return;

      const label = mapDowToLabel[d.getDay()];
      if (label) contDiaSem[label] += 1;
    });

    const maxDia = Math.max(1, ...diasOrdem.map((d) => contDiaSem[d]));
    const pres = diasOrdem.map((dia) =>
      Math.round((contDiaSem[dia] / maxDia) * 100)
    );

    const agora = new Date();
    const hojeStr = agora.toLocaleDateString("pt-BR");

    const contHoje = {};
    movs.forEach((m) => {
      if (m.data === hojeStr) {
        contHoje[m.usuario] = (contHoje[m.usuario] || 0) + 1;
      }
    });
    const dentroHoje = Object.values(contHoje).filter((c) => c % 2 === 1).length;

    return {
      presencasSemana: pres,
      semanaResumo: diasOrdem.map((dia) => ({
        dia,
        presente: contDiaSem[dia] > 0,
      })),
      entradasVsSaidas: [
        { label: "Entradas", value: movs.length },
        { label: "Saídas", value: Math.max(movs.length - dentroHoje, 0) },
      ],
      presencasAbsolutas: contDiaSem,
    };
  }, [movs]);

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

                  return (
                    <Bar
                      key={dia}
                      value={v}
                      data-count={valorReal}
                      title={`${dia}: ${valorReal} movimentações`}
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

          {/* Gráfico 2 */}
          <ChartCard>
            <ChartTitle>Entradas x Saídas</ChartTitle>
            <ChartArea>
              <BarRow>
                {entradasVsSaidas.map((item) => (
                  <Bar key={item.label} value={item.value * 2} />
                ))}
              </BarRow>

              <BarLabelRow>
                {entradasVsSaidas.map((item) => (
                  <BarLabel key={item.label}>{item.label}</BarLabel>
                ))}
              </BarLabelRow>
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

        {/* Resumo Semana */}
        <WeekSummaryCard>
          <WeekSummaryTitle>Resumo da Semana</WeekSummaryTitle>

          <WeekDaysRow>
            {semanaResumo.map((d) => (
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
