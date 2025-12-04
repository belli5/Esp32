import React, { useState, useEffect } from "react";
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
  StepsGrid,
  StepCard,
  StepHeader,
  StepTitle,
  StepDescription,
  StepStatusRow,
  LedDot,
  StepStatusText,
  StepButtonsRow,
  StepButton,
  GlobalStatusArea,
  GlobalStatusBadge,
  GlobalStatusText,
  BackButton,
} from "./saida.styles";

const MQTT_URL = "ws://172.20.10.2:9001";
const TOPIC_CMD = "portaria/comandos";
const TOPIC_STATUS = "portaria/status";

export default function Saida() {
  const navigate = useNavigate();
  const location = useLocation();

  const activeIndex = location.pathname === "/cadastro" ? 1 : 0;

  // agora só 2 etapas: funcionário e responsável
  const [employeeStatus, setEmployeeStatus] = useState("waiting");
  const [parentStatus, setParentStatus] = useState("idle");
  const [client, setClient] = useState(null);

  useEffect(() => {
    const c = mqtt.connect(MQTT_URL, {
      clientId: "front-saida-" + Math.random().toString(16).slice(2),
    });

    setClient(c);

    c.on("connect", () => {
      console.log("MQTT conectado (saida)");
      c.subscribe(TOPIC_STATUS);

      const payload = JSON.stringify({
        cmd: "start_saida",
      });
      c.publish(TOPIC_CMD, payload);
      console.log("Comando start_saida enviado:", payload);
    });

    c.on("message", (topic, payload) => {
      if (topic !== TOPIC_STATUS) return;

      try {
        const msg = JSON.parse(payload.toString());
        console.log("STATUS MQTT SAIDA:", msg);

        if (msg.context !== "saida") return;

        if (msg.step === "employee" && msg.status) {
          setEmployeeStatus(msg.status);
        }

        if (msg.step === "parent" && msg.status) {
          setParentStatus(msg.status);
        }
      } catch (err) {
        console.error("Erro ao parsear STATUS saida:", err);
      }
    });

    return () => {
      console.log("Encerrando conexão MQTT (saida)");
      c.end();
    };
  }, []);

  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

  const allOk = employeeStatus === "success" && parentStatus === "success";

  const canEmployee = true;
  const canParent = employeeStatus === "success";

  const textByStatus = {
    idle: "Aguardando início desta etapa.",
    waiting: "Aproxime o cartão do leitor.",
    success: "Cartão validado com sucesso.",
    error: "Erro na leitura. Tente novamente.",
  };

  // BOTÕES DE SIMULAÇÃO (opcionais, só mexem no estado local)
  function okEmployee() {
    if (!canEmployee) return;
    setEmployeeStatus("success");
    if (parentStatus === "idle") setParentStatus("waiting");
  }
  function errorEmployee() {
    if (!canEmployee) return;
    setEmployeeStatus("error");
  }

  function okParent() {
    if (!canParent) return;
    setParentStatus("success");
  }
  function errorParent() {
    if (!canParent) return;
    setParentStatus("error");
  }

  return (
    <PageWrapper>
      <Card>
        <BackButton onClick={goHome}>← home</BackButton>

        {/* Tabs topo */}
        <TabsBar>
          <TabsTrack>
            <Slider activeIndex={activeIndex} />

            <TabButton type="button" active={activeIndex === 0} onClick={goHome}>
              Home
            </TabButton>

            <TabButton
              type="button"
              active={activeIndex === 1}
              onClick={goCadastro}
            >
              Cadastro
            </TabButton>

            <TabButton
              type="button"
              active={activeIndex === 2}
              onClick={goDashboard}
            >
              Dashboard
            </TabButton>
          </TabsTrack>
        </TabsBar>

        <Title>Cartões de Saída</Title>
        <Subtitle>
          Leia o cartão do funcionário e do responsável para liberar a saída da escola.
        </Subtitle>

        <StepsGrid>
          {/* Etapa 1 — Funcionário */}
          <StepCard>
            <StepHeader>Etapa 1</StepHeader>
            <StepTitle>Funcionário da Instituição</StepTitle>
            <StepDescription>
              O colaborador confirma a liberação da saída utilizando seu cartão.
            </StepDescription>

            <StepStatusRow>
              <LedDot status={employeeStatus} />
              <StepStatusText>{textByStatus[employeeStatus]}</StepStatusText>
            </StepStatusRow>

          </StepCard>

          {/* Etapa 2 — Pai / Responsável */}
          <StepCard disabled={!canParent}>
            <StepHeader>Etapa 2</StepHeader>
            <StepTitle>Pai / Responsável</StepTitle>
            <StepDescription>
              O responsável confirma que está retirando o aluno da escola.
            </StepDescription>

            <StepStatusRow>
              <LedDot status={canParent ? parentStatus : "idle"} />
              <StepStatusText>
                {canParent
                  ? textByStatus[parentStatus]
                  : "Aguarde o funcionário validar a saída."}
              </StepStatusText>
            </StepStatusRow>

          </StepCard>
        </StepsGrid>

        <GlobalStatusArea status={allOk ? "success" : "waiting"}>
          <GlobalStatusBadge status={allOk ? "success" : "waiting"}>
            {allOk ? "Saída liberada" : "Fluxo em andamento"}
          </GlobalStatusBadge>

          <GlobalStatusText>
            {allOk
              ? "Funcionário e responsável confirmados. Saída autorizada."
              : "Conclua as duas etapas para liberar a saída do aluno."}
          </GlobalStatusText>
        </GlobalStatusArea>
      </Card>
    </PageWrapper>
  );
}
