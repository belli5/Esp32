import React, { useState } from "react";
import { useNavigate, useLocation } from "react-router-dom";
import mqtt from "mqtt";

const MQTT_URL = "ws://172.20.10.2:9001"; // porta de WebSocket do broker
const TOPIC_CMD = "portaria/comandos";
const TOPIC_STATUS = "portaria/status";


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

export default function Cadastrar() {
  const navigate = useNavigate();
  const location = useLocation();

  const activeIndex = location.pathname === "/cadastro" ? 1 : 0;

  const [mode, setMode] = useState(null);             // 'parent' | 'employee'
  const [status, setStatus] = useState("idle");       // 'idle' | 'waiting' | 'success' | 'error'
  const [client, setClient] = useState(null);
  const goHome = () => navigate("/");
  const goCadastro = () => navigate("/cadastro");
  const goDashboard = () => navigate("/dashboard");

    React.useEffect(() => {
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
        if (msg.context === "cadastro") {
          // status vindo da ESP32
          if (msg.status === "waiting") setStatus("waiting");
          if (msg.status === "success") setStatus("success");
          if (msg.status === "error") setStatus("error");
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
    success: "Cartão lido com sucesso! Dados prontos para salvar.",
    error: "Falha na leitura do cartão. Tente aproximar novamente.",
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

          {/* Controles simples para simular leitura */}
          <SimulateRow>
            <SimButton onClick={() => setStatus("success")}>
              Simular leitura OK
            </SimButton>
            <SimButton onClick={() => setStatus("error")}>
              Simular erro
            </SimButton>
            <SimButton onClick={() => setStatus("waiting")}>
              Voltar a aguardar
            </SimButton>
          </SimulateRow>
        </LedArea>
      </Card>
    </PageWrapper>
  );
}
