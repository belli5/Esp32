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
  OptionsRow,
  OptionButton,
  OptionTitle,
  OptionDescription,
  LedArea,
  LedRow,
  LedDot,
  LedText,
  SimulateRow,
  SimButton,
} from "./cadastro.styles";

const MQTT_URL = "ws://172.20.10.2:9001"; // porta de WebSocket do broker
const TOPIC_CMD = "portaria/comandos";
const TOPIC_STATUS = "portaria/status";

export default function Cadastrar() {
  const navigate = useNavigate();
  const location = useLocation();

  const activeIndex = location.pathname === "/cadastro" ? 1 : 0;

  const [mode, setMode] = useState(null); // 'parent' | 'employee'
  const [status, setStatus] = useState("idle"); // 'idle' | 'waiting' | 'success' | 'error' | 'exists'
  const [client, setClient] = useState(null);

  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

  useEffect(() => {
    // conecta no broker via WebSocket
    const c = mqtt.connect(MQTT_URL, {
      clientId: "front-web-" + Math.random().toString(16).slice(2),
    });

    setClient(c);

    c.on("connect", () => {
      console.log("MQTT conectado no front");
      c.subscribe(TOPIC_STATUS);
    });

    c.on("message", (topic, payload) => {
      if (topic !== TOPIC_STATUS) return;
      try {
        const msg = JSON.parse(payload.toString());
        console.log("STATUS MQTT:", msg);

        if (msg.context === "cadastro") {
          // 1) se vier um status direto (waiting, success, error, exists)
          if (msg.status) {
            setStatus(msg.status);
            return;
          }

          // 2) fallback usando event, se vier no formato antigo
          if (msg.event === "cadastro_success") {
            setStatus("success");
          } else if (msg.event === "cadastro_already_registered") {
            setStatus("exists");
          } else if (msg.event === "cadastro_error") {
            setStatus("error");
          } else if (msg.event === "cadastro_start") {
            setStatus("waiting");
          }
        }
      } catch (e) {
        console.error("Erro ao parsear status MQTT:", e);
      }
    });

    c.on("error", (err) => {
      console.error("Erro MQTT front:", err);
    });

    return () => {
      c.end(true);
    };
  }, []);

  function selectMode(nextMode) {
    setMode(nextMode);
    setStatus("waiting");

    if (client && client.connected) {
      const payload = JSON.stringify({
        cmd: "start_register",
        tipo: nextMode,
      });
      client.publish(TOPIC_CMD, payload);
      console.log("Comando de cadastro enviado:", payload);
    } else {
      console.warn("MQTT ainda não conectado no front");
    }
  }

  const labelByMode = {
    parent: "Aguardando cartão para associar ao pai/responsável...",
    employee: "Aguardando cartão do funcionário...",
  };

    const statusTextByStatus = {
    idle: "Selecione uma opção para iniciar o cadastro.",
    waiting: mode ? labelByMode[mode] : "Aguardando cartão...",
    success: "Usuário cadastrado com sucesso!",
    exists: "Usuário já cadastrado.",              // 👈 NOVO
    error: "Falha no cadastro. Tente aproximar novamente.",
  };

  return (
    <PageWrapper>
      <Card>
        {/* Tabs topo */}
        <TabsBar>
          <TabsTrack>
            <Slider activeIndex={activeIndex} />

            <TabButton
              type="button"
              active={activeIndex === 0}
              onClick={goHome}
            >
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

        <Title>Cadastro de Cartões</Title>
        <Subtitle>
          Escolha o tipo de cadastro e aproxime o cartão do leitor.
        </Subtitle>

        <OptionsRow>
          <OptionButton
            active={mode === "parent"}
            onClick={() => selectMode("parent")}
          >
            <OptionTitle>Cadastrar Responsável</OptionTitle>
            <OptionDescription>
              Associar um responsável a um filho já cadastrado via cartão.
            </OptionDescription>
          </OptionButton>

          <OptionButton
            active={mode === "employee"}
            onClick={() => selectMode("employee")}
          >
            <OptionTitle>Funcionário da Instituição</OptionTitle>
            <OptionDescription>
              Cadastrar cartão de colaboradores e equipe interna.
            </OptionDescription>
          </OptionButton>
        </OptionsRow>

        <LedArea>
          <LedRow>
            <LedDot status={status} />
            <LedText>{statusTextByStatus[status]}</LedText>
          </LedRow>

        </LedArea>
      </Card>
    </PageWrapper>
  );
}
